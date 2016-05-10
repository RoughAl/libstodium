/**
 * This file wraps calls to methods in libsodium using ByteBuffer instances to
 * provide access to the data for the operations.
 *
 * @author Jan van de Molengraft [jan@artemisc.eu]
 */

// Required headers
#include <jni.h>
#include <stdbool.h>
#include "sodium.h"

#define STODIUM_JNI(type, method) JNIEXPORT type JNICALL Java_eu_artemisc_stodium_StodiumJNI_##method

/**
 * STODIUM_CONSTANT is a utility macro for crypto_primitive_sthsize() methods.
 * It can be used for every method that takes 0 arguments and returns a jint
 * value.
 *
 * @group:     the group the primitive belongs to (e.g. core, stream, aead)
 * @primitive: the name of the primitive (e.g. ed25519, blake2b)
 * @constant:  the name of the constant in lower case (e.g. inputbytes, constbytes)
 */
#define STODIUM_CONSTANT(group, primitive, constant) \
    STODIUM_JNI(jint, crypto_1##group##_1##primitive##_1##constant) (JNIEnv *jenv, jclass jcls) { \
        return (jint) crypto_##group##_##primitive##_##constant (); }

/**
 * STODIUM_CONSTANT_STR does the same as STODIUM_CONSTANT, except it returns a
 * Java String instead of a jint. Used for crypto_*_primitive() wrappers.
 */
#define STODIUM_CONSTANT_STR(group) \
    STODIUM_JNI(jstring, crypto_1##group##_1primitive) (JNIEnv *jenv, jclass jcls) { \
        return (*jenv)->NewStringUTF(jenv, crypto_##group##_primitive ()); }

/**
 * STODIUM_CONSTANT_HL does the same as STODIUM_CONSTANT, except it works for
 * High Level API's (without explicit implementation identifiers), excluding the
 * primitive from the argument list.
 */
#define STODIUM_CONSTANT_HL(group, constant) \
    STODIUM_JNI(jint, crypto_1##group##_1##constant) (JNIEnv *jenv, jclass jcls) { \
        return (jint) crypto_##group##_##constant (); }

/**
 * AS_INPUT, AS_OUTPUT, AS_INPUT_LEN and AS_OUTPUT_LEN are utility macros to
 * reduce the effort of writing casting code and buffer references in every
 * wrapper function.
 */
#define AS_INPUT(type, buffer)      ((const type *) (buffer.content + buffer.offset))
#define AS_OUTPUT(type, buffer)     ((type *)       (buffer.content + buffer.offset))

#define AS_INPUT_LEN(type, buffer)  ((type)   (buffer.capacity))
#define AS_OUTPUT_LEN(type, buffer) ((type *) (buffer.capacity))

/**
 * Beginning of the real C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * These static variables are used to hold cached references to Java values in
 * order to reduce the amount of calls made to the JVM from the native code.
 */
static jclass    stodium_g_byte_buffer_class;
static jmethodID stodium_g_byte_buffer_method_array;
static jmethodID stodium_g_byte_buffer_method_array_offset;
static jmethodID stodium_g_byte_buffer_method_remaining;

/**
 * JNI_OnLoad caches the methods called on indirect (backing array) versions of
 * ByteBuffers passed to Stodium methods, to avoid repreated calls to
 * GetMethodID.
 */
jint JNI_OnLoad(JavaVM* jvm, void* reserved) {
    JNIEnv *jenv;
    if ((*jvm)->GetEnv(jvm, (void**)(&jenv), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    stodium_g_byte_buffer_class = (*jenv)->FindClass(jenv, "java/nio/ByteBuffer");
    if ((*jenv)->ExceptionCheck(jenv)) {
        return -1;
    }

    stodium_g_byte_buffer_method_array = (*jenv)->GetMethodID(jenv, stodium_g_byte_buffer_class, "array", "()[B");
    if ((*jenv)->ExceptionCheck(jenv)) {
        return -1;
    }

    stodium_g_byte_buffer_method_array_offset = (*jenv)->GetMethodID(jenv, stodium_g_byte_buffer_class, "arrayOffset", "()I");
    if ((*jenv)->ExceptionCheck(jenv)) {
        return -1;
    }

    stodium_g_byte_buffer_method_remaining = (*jenv)->GetMethodID(jenv, stodium_g_byte_buffer_class, "remaining", "()I");
    if ((*jenv)->ExceptionCheck(jenv)) {
        return -1;
    }

    return JNI_VERSION_1_6;
}

/**
 * stodium_buffers represent a C-accessible link to the array held by a
 * ByteBuffer instance. The methods working with stodium_buffers use the fields
 * of the struct to determine whether the JNI methods should be used to manage a
 * Direct buffer, or whether an underling jbyteArray should be addressed.
 */
typedef struct stodium_buffers {
    unsigned char *content;
    size_t         offset;
    size_t         capacity;
    bool           is_direct;
    jbyteArray     backing_array; // Only defined if the buffer was not direct
} stodium_buffer;

/**
 *
 */
void stodium_get_buffer(JNIEnv *jenv, stodium_buffer *dst, jobject jbuffer) {
    if (jbuffer == NULL) {
        dst->content   = 0;
        dst->offset    = 0;
        dst->capacity  = 0;
        dst->is_direct = true; // A null buffer can be treated as direct
        return;
    }

    dst->content = (unsigned char *) (*jenv)->GetDirectBufferAddress(jenv, jbuffer);
    if (dst->content != NULL) {
        dst->offset    = 0;
        dst->capacity  = (size_t) (*jenv)->GetDirectBufferCapacity(jenv, jbuffer);
        dst->is_direct = true;
        return;
    }

    // indirect (backing array). HALP
    dst->backing_array = (jbyteArray) (*jenv)->CallObjectMethod(jenv, jbuffer, stodium_g_byte_buffer_method_array);
    dst->content       = (unsigned char *) (*jenv)->GetByteArrayElements(jenv, dst->backing_array, NULL);
    dst->offset        = (size_t) (*jenv)->CallIntMethod(jenv, jbuffer, stodium_g_byte_buffer_method_array_offset);
    dst->capacity      = (size_t) (*jenv)->CallIntMethod(jenv, jbuffer, stodium_g_byte_buffer_method_remaining);
    dst->is_direct     = false;
    return;
}

/**
 *
 */
void stodium_release_output(JNIEnv *jenv, jobject output, stodium_buffer *buffer) {
    if (buffer->is_direct) {
        return; // No need for copying
    }
    
    // Release with copying of the native buffer
    (*jenv)->ReleaseByteArrayElements(jenv, buffer->backing_array, (jbyte *) (buffer->content), 0);
}

/**
 *
 */
void stodium_release_input(JNIEnv *jenv, jobject output, stodium_buffer *buffer) {
    if (buffer->is_direct || buffer->content == 0) {
        return; // No need for copying or releasing
    }

    // Release without copying the native buffer
    (*jenv)->ReleaseByteArrayElements(jenv, buffer->backing_array, (jbyte *) (buffer->content), JNI_ABORT);
}

/**
 * Libstodium init method, caches common values for accessing ByteBuffer data.
 */
STODIUM_JNI(jint, stodium_1init) (JNIEnv *jenv, jclass jcls) {
    if (sodium_init() == -1) {
        return -1;
    }

    // TODO JNI cachings?
    return 0;
}

/** ****************************************************************************
 *
 * Libsodium library methods
 *
 **************************************************************************** */

STODIUM_JNI(jint, sodium_1init) (JNIEnv *jenv, jclass jcls) {
    return (jint) sodium_init();
}

/** ****************************************************************************
 *
 * CORE - HSALSA20
 *
 **************************************************************************** */

STODIUM_CONSTANT(core, hsalsa20, outputbytes)
STODIUM_CONSTANT(core, hsalsa20, inputbytes)
STODIUM_CONSTANT(core, hsalsa20, keybytes)
STODIUM_CONSTANT(core, hsalsa20, constbytes)

STODIUM_JNI(jint, crypto_1core_1hsalsa20) (JNIEnv *jenv, jclass jcls,
        jobject dst,
        jobject src,
        jobject key,
        jobject constant) {
    stodium_buffer dst_buffer, src_buffer, key_buffer, const_buffer;
    stodium_get_buffer(jenv, &dst_buffer,   dst);
    stodium_get_buffer(jenv, &src_buffer,   src);
    stodium_get_buffer(jenv, &key_buffer,   key);
    stodium_get_buffer(jenv, &const_buffer, constant);

    jint result = (jint) crypto_core_hsalsa20(
            AS_OUTPUT(unsigned char, dst_buffer),
            AS_INPUT(unsigned char, src_buffer),
            AS_INPUT(unsigned char, key_buffer),
            AS_INPUT(unsigned char, const_buffer));

    stodium_release_output(jenv, dst, &dst_buffer);
    stodium_release_input(jenv, src, &src_buffer);
    stodium_release_input(jenv, key, &key_buffer);
    stodium_release_input(jenv, constant, &const_buffer);
    
    return result;
}

/** ****************************************************************************
 *
 * SCALARMULT - Curve25519
 *
 **************************************************************************** */
STODIUM_CONSTANT_STR(scalarmult)

STODIUM_CONSTANT(scalarmult, curve25519, bytes)
STODIUM_CONSTANT(scalarmult, curve25519, scalarbytes)

STODIUM_JNI(jint, crypto_1scalarmult_1curve25519) (JNIEnv *jenv, jclass jcls,
        jobject dst,
        jobject priv,
        jobject pub) {
    stodium_buffer dst_buffer, priv_buffer, pub_buffer;
    stodium_get_buffer(jenv, &dst_buffer,  dst);
    stodium_get_buffer(jenv, &priv_buffer, priv);
    stodium_get_buffer(jenv, &pub_buffer,  pub);

    jint result = (jint) crypto_scalarmult_curve25519(
            AS_OUTPUT(unsigned char, dst_buffer),
            AS_INPUT(unsigned char,  priv_buffer),
            AS_INPUT(unsigned char,  pub_buffer));

    stodium_release_output(jenv, dst, &dst_buffer);
    stodium_release_input(jenv, priv, &priv_buffer);
    stodium_release_input(jenv, pub, &pub_buffer);
    
    return result;
}


STODIUM_JNI(jint, crypto_1scalarmult_1curve25519_1base) (JNIEnv *jenv, jclass jcls,
        jobject dst,
        jobject src) {
    stodium_buffer dst_buffer, src_buffer;
    stodium_get_buffer(jenv, &dst_buffer, dst);
    stodium_get_buffer(jenv, &src_buffer, src);
    
    jint result = (jint) crypto_scalarmult_curve25519_base(
            AS_OUTPUT(unsigned char, dst_buffer),
            AS_INPUT(unsigned char, src_buffer));

    stodium_release_output(jenv, dst, &dst_buffer);
    stodium_release_input(jenv, src, &src_buffer);
    
    return result;
}

#ifdef __cplusplus
}
#endif
