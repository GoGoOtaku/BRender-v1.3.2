/*
 * Device pixelmap implementation, front/screen edition.
 */

#include "drv.h"
#include <brassert.h>

/*
 * Default dispatch table for device (defined at end of file)
 */
static const struct br_device_pixelmap_dispatch devicePixelmapFrontDispatch;

// static br_error custom_query(br_value* pvalue, void** extra, br_size_t* pextra_size, void* block, struct br_tv_template_entry* tep) {
//     const br_device_pixelmap* self = block;

//     switch (tep->token) {
//     case BRT_OPENGL_EXT_PROCS_P:
//         pvalue->p = (void*)&self->asFront.ext_procs;
//         break;
//     default:
//         return BRE_UNKNOWN;
//     }

//     return BRE_OK;
// }

// static const br_tv_custom custom = {
//     .query = custom_query,
//     .set = NULL,
//     .extra_size = NULL,
// };

/*
 * Device pixelmap info. template
 */
#define F(f) offsetof(br_device_pixelmap, f)
#define FF(f) offsetof(br_device_pixelmap, asFront.f)
static struct br_tv_template_entry devicePixelmapFrontTemplateEntries[] = {
    { BRT(WIDTH_I32), F(pm_width), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U16, 0 },
    { BRT(HEIGHT_I32), F(pm_height), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U16, 0 },
    { BRT(PIXEL_TYPE_U8), F(pm_type), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U8, 0 },
    { BRT(OUTPUT_FACILITY_O), F(output_facility), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(FACILITY_O), F(output_facility), BRTV_QUERY, BRTV_CONV_COPY, 0 },
    { BRT(IDENTIFIER_CSTR), F(pm_identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(MSAA_SAMPLES_I32), F(msaa_samples), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(OPENGL_CALLBACKS_P), 0, BRTV_QUERY | BRTV_ALL, BRTV_CONV_DIRECT },
    { BRT(OPENGL_VERSION_CSTR), FF(gl_version), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(OPENGL_VENDOR_CSTR), FF(gl_vendor), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(OPENGL_RENDERER_CSTR), FF(gl_renderer), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },

    { DEV(OPENGL_NUM_EXTENSIONS_I32), FF(gl_num_extensions), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { DEV(OPENGL_EXTENSIONS_PL), FF(gl_extensions), BRTV_QUERY | BRTV_ALL, BRTV_CONV_LIST, 0 },
    {
        BRT_CLUT_O,
        0,
        F(clut),
        BRTV_QUERY | BRTV_ALL,
        BRTV_CONV_COPY,
    }
};
#undef FF
#undef F

struct pixelmapNewTokens {
    br_int_32 width;
    br_int_32 height;
    br_int_32 pixel_bits;
    br_uint_8 pixel_type;
    int msaa_samples;
    br_device_gl_callback_procs* callbacks;
    const char* vertex_shader;
    const char* fragment_shader;
};

#define F(f) offsetof(struct pixelmapNewTokens, f)
static struct br_tv_template_entry pixelmapNewTemplateEntries[] = {
    { BRT(WIDTH_I32), F(width), BRTV_SET, BRTV_CONV_COPY },
    { BRT(HEIGHT_I32), F(height), BRTV_SET, BRTV_CONV_COPY },
    { BRT(PIXEL_BITS_I32), F(pixel_bits), BRTV_SET, BRTV_CONV_COPY },
    { BRT(PIXEL_TYPE_U8), F(pixel_type), BRTV_SET, BRTV_CONV_COPY },
    { BRT(MSAA_SAMPLES_I32), F(msaa_samples), BRTV_SET, BRTV_CONV_COPY },
    { BRT(OPENGL_CALLBACKS_P), F(callbacks), BRTV_SET, BRTV_CONV_COPY },
    { BRT(OPENGL_FRAGMENT_SHADER_STR), F(fragment_shader), BRTV_SET, BRTV_CONV_COPY }
};
#undef F

static void SetupFullScreenRectGeometry(br_device_pixelmap* self) {
    float vertices[] = {
        // positions          // colors           // texture coords
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // top right
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // bottom left
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    GLuint vbo;
    glGenVertexArrays(1, &self->asFront.screen_buffer_vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &self->asFront.screen_buffer_ebo);

    glBindVertexArray(self->asFront.screen_buffer_vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->asFront.screen_buffer_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    GL_CHECK_ERROR();
}

void RenderFullScreenTextureToFrameBuffer(br_device_pixelmap* self, GLuint textureId, GLuint fb, int flipVertically, int discardPurplePixels) {

    int x, y;
    float width_m, height_m;
    DevicePixelmapGLGetViewport(self, &x, &y, &width_m, &height_m);
    glViewport(x, y, self->pm_width * width_m, self->pm_height * height_m);

    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBindVertexArray(self->asFront.screen_buffer_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->asFront.screen_buffer_ebo);
    glUseProgram(self->asFront.video.defaultProgram.program);
    glUniform1f(self->asFront.video.defaultProgram.uFlipVertically, (float)flipVertically);
    glUniform1i(self->asFront.video.defaultProgram.uDiscardPurplePixels, discardPurplePixels);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_ERROR();
}

br_device_pixelmap* DevicePixelmapGLAllocateFront(br_device* dev, br_output_facility* outfcty, br_token_value* tv) {
    br_device_pixelmap* self;
    br_int_32 count;
    GLint red_bits = 0, grn_bits = 0, blu_bits = 0, alpha_bits = 0;
    struct pixelmapNewTokens pt = {
        .width = -1,
        .height = -1,
        .pixel_bits = -1,
        .pixel_type = BR_PMT_MAX,
        .msaa_samples = 0,
        .callbacks = NULL,
        .vertex_shader = NULL,
        .fragment_shader = NULL
    };
    char tmp[80];

    if (dev->templates.pixelmapNewTemplate == NULL) {
        dev->templates.pixelmapNewTemplate = BrTVTemplateAllocate(dev, pixelmapNewTemplateEntries,
            BR_ASIZE(pixelmapNewTemplateEntries));
    }

    BrTokenValueSetMany(&pt, &count, NULL, tv, dev->templates.pixelmapNewTemplate);

    if(pt.callbacks == NULL || pt.width <= 0 || pt.height <= 0)
        return NULL;

    if ((pt.pixel_type = DeviceGLTypeOrBits(pt.pixel_type, pt.pixel_bits)) == BR_PMT_MAX)
        return NULL;

    self = BrResAllocate(dev->res, sizeof(br_device_pixelmap), BR_MEMORY_OBJECT);

    BrSprintfN(tmp, sizeof(tmp) - 1, "OpenGL:Screen:%dx%d", pt.width, pt.height);
    self->pm_identifier = BrResStrDup(self, tmp);
    self->dispatch = &devicePixelmapFrontDispatch;
    self->device = dev;
    self->output_facility = outfcty;
    self->use_type = BRT_NONE;
    self->msaa_samples = pt.msaa_samples;
    self->screen = self;
    self->clut = dev->clut;

    self->pm_type = pt.pixel_type;
    self->pm_width = pt.width;
    self->pm_height = pt.height;
    self->pm_flags |= BR_PMF_NO_ACCESS;

    /*
     * Make a copy, so they can't switch things out from under us.
     */
    self->asFront.callbacks = *pt.callbacks;

    if (gladLoadGLLoader(DevicePixelmapGLGetGetProcAddress(self)) == 0) {
        BR_ERROR("GLREND: Unable to load OpenGL functions.");
        goto cleanup_context;
    }

    self->asFront.gl_version = BrResStrDup(self, (char*)glGetString(GL_VERSION));
    self->asFront.gl_vendor = BrResStrDup(self, (char*)glGetString(GL_VENDOR));
    self->asFront.gl_renderer = BrResStrDup(self, (char*)glGetString(GL_RENDERER));

    BrLogPrintf("GLREND: OpenGL Version  = %s\n", self->asFront.gl_version);
    BrLogPrintf("GLREND: OpenGL Vendor   = %s\n", self->asFront.gl_vendor);
    BrLogPrintf("GLREND: OpenGL Renderer = %s\n", self->asFront.gl_renderer);

    if (GLVersion.major < 3 || (GLVersion.major == 3 && GLVersion.minor < 1)) {
        BR_FATAL2("GLREND: Got OpenGL %d.%d context, expected 3.1", GLVersion.major, GLVersion.minor);
        goto cleanup_context;
    }

    /*
     * Get a copy of the extension list.
     * NULL-terminate so we can expose it as a BRT_POINTER_LIST.
     */
    glGetIntegerv(GL_NUM_EXTENSIONS, &self->asFront.gl_num_extensions);

    self->asFront.gl_extensions = BrResAllocate(self, sizeof(char*) * (self->asFront.gl_num_extensions + 1), BR_MEMORY_DRIVER);
    for (GLuint i = 0; i < self->asFront.gl_num_extensions; ++i) {
        const GLubyte* ext = glGetStringi(GL_EXTENSIONS, i);
        self->asFront.gl_extensions[i] = BrResStrDup(self->asFront.gl_extensions, (char*)ext);
    }
    self->asFront.gl_extensions[self->asFront.gl_num_extensions] = NULL;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    if (VIDEO_Open(&self->asFront.video, pt.vertex_shader, pt.fragment_shader) == NULL) {
        /*
         * If this fails we can run our regular cleanup.
         */
        BrResFree(self);
        return NULL;
    }
    self->asFront.tex_white = DeviceGLBuildWhiteTexture();
    self->asFront.tex_checkerboard = DeviceGLBuildCheckerboardTexture();

    self->asFront.num_refs = 0;

    SetupFullScreenRectGeometry(self);

    ObjectContainerAddFront(self->output_facility, (br_object*)self);
    GL_CHECK_ERROR();
    return self;

cleanup_context:
    DevicePixelmapGLFree(self);
    BrResFreeNoCallback(self);

    return NULL;
}

static void BR_CMETHOD_DECL(br_device_pixelmap_glf, free)(br_object* _self) {
    br_device_pixelmap* self = (br_device_pixelmap*)_self;

    //BrLogPrintf("GLREND: Freeing %s\n", self->pm_identifier);

    //UASSERT(self->asFront.num_refs == 0);

    glDeleteTextures(1, &self->asFront.tex_checkerboard);
    glDeleteTextures(1, &self->asFront.tex_white);

    VIDEO_Close(&self->asFront.video);

    // TODO: uncomment
    // ObjectContainerRemove(self->output_facility, (br_object*)self);

    DevicePixelmapGLFree(self);

    BrResFreeNoCallback(self);
}

const char* BR_CMETHOD_DECL(br_device_pixelmap_glf, identifier)(br_object* self) {
    return ((br_device_pixelmap*)self)->pm_identifier;
}

br_token BR_CMETHOD_DECL(br_device_pixelmap_glf, type)(br_object* self) {
    (void)self;
    return BRT_DEVICE_PIXELMAP;
}

br_boolean BR_CMETHOD_DECL(br_device_pixelmap_glf, isType)(br_object* self, br_token t) {
    (void)self;
    return (t == BRT_DEVICE_PIXELMAP) || (t == BRT_OBJECT);
}

br_device* BR_CMETHOD_DECL(br_device_pixelmap_glf, device)(br_object* self) {
    (void)self;
    return ((br_device_pixelmap*)self)->device;
}

br_size_t BR_CMETHOD_DECL(br_device_pixelmap_glf, space)(br_object* self) {
    (void)self;
    return sizeof(br_device_pixelmap);
}

struct br_tv_template* BR_CMETHOD_DECL(br_device_pixelmap_glf, templateQuery)(br_object* _self) {
    br_device_pixelmap* self = (br_device_pixelmap*)_self;

    if (self->device->templates.devicePixelmapFrontTemplate == NULL)
        self->device->templates.devicePixelmapFrontTemplate = BrTVTemplateAllocate(
            self->device, devicePixelmapFrontTemplateEntries, BR_ASIZE(devicePixelmapFrontTemplateEntries));

    return self->device->templates.devicePixelmapFrontTemplate;
}

br_error BR_CMETHOD_DECL(br_device_pixelmap_glf, resize)(br_device_pixelmap* self, br_int_32 width, br_int_32 height) {
    br_error err;

    self->pm_width = width;
    self->pm_height = height;
    return BRE_OK;
}

br_error BR_CMETHOD_DECL(br_device_pixelmap_glf, doubleBuffer)(br_device_pixelmap* self, br_device_pixelmap* src) {

    /*
     * Ignore self-blit.
     */
    if (self == src)
        return BRE_OK;

    if (ObjectDevice(src) != self->device)
        return BRE_UNSUPPORTED;

    if (self->use_type != BRT_NONE || src->use_type != BRT_OFFSCREEN) {
        return BRE_UNSUPPORTED;
    }

    // ensure all dirty pixel writes have been flushed
    BrPixelmapFlush((br_pixelmap*)src);

    DevicePixelmapGLSwapBuffers(self);

    // prep for next frame
    BrRendererFrameBegin();

    GL_CHECK_ERROR();

    return BRE_OK;
}


/*
 * Default dispatch table for device pixelmap
 */
static const struct br_device_pixelmap_dispatch devicePixelmapFrontDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_device_pixelmap_glf, free),
    ._identifier = BR_CMETHOD_REF(br_device_pixelmap_glf, identifier),
    ._type = BR_CMETHOD_REF(br_device_pixelmap_glf, type),
    ._isType = BR_CMETHOD_REF(br_device_pixelmap_glf, isType),
    ._device = BR_CMETHOD_REF(br_device_pixelmap_glf, device),
    ._space = BR_CMETHOD_REF(br_device_pixelmap_glf, space),

    ._templateQuery = BR_CMETHOD_REF(br_device_pixelmap_glf, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._validSource = BR_CMETHOD_REF(br_device_pixelmap_mem, validSource),
    ._resize = BR_CMETHOD_REF(br_device_pixelmap_glf, resize),
    ._match = BR_CMETHOD_REF(br_device_pixelmap_gl, match),
    ._allocateSub = BR_CMETHOD_REF(br_device_pixelmap_fail, allocateSub),

    ._copy = BR_CMETHOD_REF(br_device_pixelmap_fail, copy),
    ._copyTo = BR_CMETHOD_REF(br_device_pixelmap_fail, copyTo),
    ._copyFrom = BR_CMETHOD_REF(br_device_pixelmap_fail, copyFrom),
    ._fill = BR_CMETHOD_REF(br_device_pixelmap_fail, fill),
    ._doubleBuffer = BR_CMETHOD_REF(br_device_pixelmap_glf, doubleBuffer),

    ._copyDirty = BR_CMETHOD_REF(br_device_pixelmap_fail, copyDirty),
    ._copyToDirty = BR_CMETHOD_REF(br_device_pixelmap_fail, copyToDirty),
    ._copyFromDirty = BR_CMETHOD_REF(br_device_pixelmap_fail, copyFromDirty),
    ._fillDirty = BR_CMETHOD_REF(br_device_pixelmap_fail, fillDirty),
    ._doubleBufferDirty = BR_CMETHOD_REF(br_device_pixelmap_fail, doubleBufferDirty),

    ._rectangle = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangle),
    ._rectangle2 = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangle2),
    ._rectangleCopy = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleCopyTo),
    ._rectangleCopyTo = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleCopyTo),
    ._rectangleCopyFrom = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleCopyFrom),
    ._rectangleStretchCopy = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleStretchCopyTo),
    ._rectangleStretchCopyTo = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleStretchCopyTo),
    ._rectangleStretchCopyFrom = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleStretchCopyFrom),
    ._rectangleFill = BR_CMETHOD_REF(br_device_pixelmap_fail, rectangleFill),
    ._pixelSet = BR_CMETHOD_REF(br_device_pixelmap_fail, pixelSet),
    ._line = BR_CMETHOD_REF(br_device_pixelmap_fail, line),
    ._copyBits = BR_CMETHOD_REF(br_device_pixelmap_fail, copyBits),

    ._text = BR_CMETHOD_REF(br_device_pixelmap_fail, text),
    ._textBounds = BR_CMETHOD_REF(br_device_pixelmap_gen, textBounds),

    ._rowSize = BR_CMETHOD_REF(br_device_pixelmap_fail, rowSize),
    ._rowQuery = BR_CMETHOD_REF(br_device_pixelmap_fail, rowQuery),
    ._rowSet = BR_CMETHOD_REF(br_device_pixelmap_fail, rowSet),

    ._pixelQuery = BR_CMETHOD_REF(br_device_pixelmap_fail, pixelQuery),
    ._pixelAddressQuery = BR_CMETHOD_REF(br_device_pixelmap_fail, pixelAddressQuery),

    ._pixelAddressSet = BR_CMETHOD_REF(br_device_pixelmap_fail, pixelAddressSet),
    ._originSet = BR_CMETHOD_REF(br_device_pixelmap_mem, originSet),

    ._flush = BR_CMETHOD_REF(br_device_pixelmap_fail, flush),
    ._synchronise = BR_CMETHOD_REF(br_device_pixelmap_fail, synchronise),
    ._directLock = BR_CMETHOD_REF(br_device_pixelmap_fail, directLock),
    ._directUnlock = BR_CMETHOD_REF(br_device_pixelmap_fail, directUnlock),
    ._getControls = BR_CMETHOD_REF(br_device_pixelmap_fail, getControls),
    ._setControls = BR_CMETHOD_REF(br_device_pixelmap_fail, setControls)
};
