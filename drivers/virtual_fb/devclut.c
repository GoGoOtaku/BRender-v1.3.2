/*
 * CLUT methods
 */
#include <stddef.h>
#include <string.h>

#include "brassert.h"
#include "drv.h"
#include "shortcut.h"

BR_RCS_ID("$Id: devclut.c 1.1 1997/12/10 16:45:31 jon Exp $");

/*
 * Default dispatch table for device_clut (defined at end of file)
 */
static const struct br_device_clut_dispatch deviceClutDispatch;

/*
 * Renderer info. template
 */
#define F(f) offsetof(struct br_device_clut, f)

static const struct br_tv_template_entry deviceClutTemplateEntries[] = {
    {
        BRT_IDENTIFIER_CSTR,
        0,
        F(identifier),
        BRTV_QUERY | BRTV_ALL,
        BRTV_CONV_COPY,
    },
};
#undef F

/*
 * Create a new device CLUT
 */
br_device_clut* DeviceClutVirtualFBAllocate(br_device* dev, char* identifier) {
    br_device_clut* self;

    self = BrResAllocate(DeviceVirtualFBResource(dev), sizeof(*self), BR_MEMORY_OBJECT_DATA);

    self->dispatch = &deviceClutDispatch;
    if (identifier)
        self->identifier = identifier;
    self->device = dev;

    ObjectContainerAddFront(dev, (br_object*)self);

    return self;
}

static void BR_CMETHOD_DECL(br_device_clut_virtualfb, free)(br_device_clut* self) {
    ObjectContainerRemove(ObjectDevice(self), (br_object*)self);

    BrResFreeNoCallback(self);
}

static br_token BR_CMETHOD_DECL(br_device_clut_virtualfb, type)(br_device_clut* self) {
    return BRT_DEVICE_CLUT;
}

static br_boolean BR_CMETHOD_DECL(br_device_clut_virtualfb, isType)(br_device_clut* self, br_token t) {
    return (t == BRT_DEVICE_CLUT) || (t == BRT_OBJECT);
}

static br_int_32 BR_CMETHOD_DECL(br_device_clut_virtualfb, space)(br_device_clut* self) {
    return sizeof(br_device_clut);
}

static struct br_tv_template* BR_CMETHOD_DECL(br_device_clut_virtualfb, queryTemplate)(br_device_clut* self) {
    if (self->device->templates.deviceClutTemplate == NULL)
        self->device->templates.deviceClutTemplate = BrTVTemplateAllocate(self->device,
            deviceClutTemplateEntries,
            BR_ASIZE(deviceClutTemplateEntries));

    return self->device->templates.deviceClutTemplate;
}

static br_error BR_CMETHOD_DECL(br_device_clut_virtualfb, entrySet)(br_device_clut* self, br_int_32 index, br_colour entry) {
    if (index < 0 || index > 255)
        return BRE_OVERFLOW;

    /*
     * Setup the palette by writing the DAC registers directly
     */
    // outp(VGA_PAL_WRITE,index);

    // outp(VGA_PAL_DATA,BR_RED(entry) >> 2);
    // outp(VGA_PAL_DATA,BR_GRN(entry) >> 2);
    // outp(VGA_PAL_DATA,BR_BLU(entry) >> 2);

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_clut_virtualfb, entryQuery)(br_device_clut* self, br_colour* entry, br_int_32 index) {
    br_uint_8 r, g, b;

    if (index < 0 || index > 255)
        return BRE_OVERFLOW;

    // /*
    //  * Read the palette by accessing the DAC registers directly
    //  */
    // outp(VGA_PAL_READ,index);

    // r = inp(VGA_PAL_DATA);
    // g = inp(VGA_PAL_DATA);
    // b = inp(VGA_PAL_DATA);

    *entry = BR_COLOUR_RGB(r, g, b);

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_clut_virtualfb, entrySetMany)(br_device_clut* self, br_int_32 index, br_int_32 count, br_colour* entries) {
    int i;

    if (index < 0 || index > 255)
        return BRE_OVERFLOW;

    if (index + count > 256)
        return BRE_OVERFLOW;

    for (i = 0; i < count; i++) {
        self->entries[index + i] = entries[i];
    }

    if (self->callbacks && self->callbacks->palette_changed) {
        self->callbacks->palette_changed(self->entries);
    }

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_clut_virtualfb, entryQueryMany)(br_device_clut* self, br_colour* entries, br_int_32 index, br_int_32 count) {
    br_uint_8 r, g, b;
    int i;

    if (index < 0 || index > 255)
        return BRE_OVERFLOW;

    if (index + count > 256)
        return BRE_OVERFLOW;

    // outp(VGA_PAL_READ,index);

    for (i = 0; i < count; i++) {
        // r = inp(VGA_PAL_DATA);
        // g = inp(VGA_PAL_DATA);
        // b = inp(VGA_PAL_DATA);

        entries[i] = BR_COLOUR_RGB(r, g, b);
    }

    return BRE_OK;
}

/*
 * Default dispatch table for device CLUT
 */
static const struct br_device_clut_dispatch deviceClutDispatch = {
    NULL,
    NULL,
    NULL,
    NULL,
    BR_CMETHOD_REF(br_device_clut_virtualfb, free),
    BR_CMETHOD_REF(br_object_virtualfb, identifier),
    BR_CMETHOD_REF(br_device_clut_virtualfb, type),
    BR_CMETHOD_REF(br_device_clut_virtualfb, isType),
    BR_CMETHOD_REF(br_object_virtualfb, device),
    BR_CMETHOD_REF(br_device_clut_virtualfb, space),

    BR_CMETHOD_REF(br_device_clut_virtualfb, queryTemplate),
    BR_CMETHOD_REF(br_object, query),
    BR_CMETHOD_REF(br_object, queryBuffer),
    BR_CMETHOD_REF(br_object, queryMany),
    BR_CMETHOD_REF(br_object, queryManySize),
    BR_CMETHOD_REF(br_object, queryAll),
    BR_CMETHOD_REF(br_object, queryAllSize),

    BR_CMETHOD_REF(br_device_clut_virtualfb, entrySet),
    BR_CMETHOD_REF(br_device_clut_virtualfb, entryQuery),
    BR_CMETHOD_REF(br_device_clut_virtualfb, entrySetMany),
    BR_CMETHOD_REF(br_device_clut_virtualfb, entryQueryMany),
};
