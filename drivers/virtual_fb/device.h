/*
 * Private device driver structure
 */
#ifndef _DEVICE_H_
#define _DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Private state of device
 */
typedef struct br_device {
    /*
     * Dispatch table
     */
    struct br_device_dispatch* dispatch;

    /*
     * Standard object identifier
     */
    char* identifier;

    /*
     * Pointer to owning device
     */
    struct br_device* device;

    /*
     * List of objects associated with this device
     */
    void* object_list;

    /*
     * Anchor for all device's resources
     */
    void* res;

    /*
     * VGA mode at startup
     */
    br_uint_16 original_mode;

    /*
     * Current mode
     */
    br_boolean active;

    br_uint_16 current_mode;

    /*
     * Default (and only) CLUT
     */
    struct br_device_clut* clut;

    struct device_templates templates;

} br_device;

/*
 * Some useful inline ops.
 */
#define DeviceVirtualFBResource(d) (((br_device*)d)->res)
#define DeviceVirtualFBClut(d) (((br_device*)d)->clut)

#ifdef __cplusplus
};
#endif
#endif
