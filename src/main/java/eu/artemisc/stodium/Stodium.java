package eu.artemisc.stodium;

import android.os.Build;
import android.support.annotation.NonNull;

import org.abstractj.kalium.Sodium;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.crypto.AEADBadTagException;

/**
 * Stodium is an abstract class with static methods. It is an attempt to
 * simplify the API generated by SWIG to a more Java-ish version, as well as add
 * some proper JavaDocs to the methods.
 *
 * All method calls are wrappers around calls to JNI implemented methods. The
 * library is aimed specifically at the android platform.
 *
 * @author Jan van de Molengraft [jan@artemisc.eu]
 */
public final class Stodium {
    // Block constructor
    private Stodium() {}

    /**
     *
     * @param status
     * @throws StodiumException
     */
    public static void checkStatus(final int status)
            throws StodiumException {
        if (status == 0) {
            return;
        }
        throw new StodiumException(
                String.format("Stodium: operation returned non-zero status %d", status));
    }

    /**
     *
     * @param status
     * @param methodDescription
     * @throws AEADBadTagException If the status value does not equal 0,
     *         indicating an invalid authentication tag was encountered.
     * @throws StodiumException If the API level does not support
     *         AEADBadTagException, the method will call
     *         {@link #checkStatus(int)} instead.
     */
    public static void checkStatusSealOpen(final int status,
                                           @NonNull final String methodDescription)
            throws AEADBadTagException, StodiumException {
        if (status == 0) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            throw new AEADBadTagException(
                    methodDescription + ": cannot open sealed box (invalid tag?)");
        } else {
            checkStatus(status);
        }
    }

    /**
     *
     * @param src
     * @param expected
     * @param constant
     * @throws ConstraintViolationException
     */
    public static void checkSize(final int src,
                                 final int expected,
                                 @NonNull final String constant)
            throws ConstraintViolationException {
        if (src == expected) {
            return;
        }
        throw new SecurityException(
                String.format("Check size failed on [%s] [expected: %d, real: %d]",
                        constant, expected, src));
    }

    /**
     *
     * @param src
     * @param lower
     * @param upper
     * @param lowerC
     * @param upperC
     * @throws ConstraintViolationException
     */
    public static void checkSize(final int src,
                                 final int lower,
                                 final int upper,
                                 @NonNull final String lowerC,
                                 @NonNull final String upperC)
            throws ConstraintViolationException {
        if (src <= upper && src >= lower) {
            return;
        }
        throw new ConstraintViolationException(
                String.format("CheckSize failed on bounds [%s, %s] [lower: %d, upper: %d, real: %d]",
                        lowerC, upperC, lower, upper, src));
    }

    /**
     *
     * @param src
     * @throws ConstraintViolationException
     */
    public static void checkPositive(final int src)
            throws ConstraintViolationException {
        if (src >= 0) {
            return;
        }
        throw new ConstraintViolationException(
                String.format("checkPositice failed [real: %d]", src));
    }

    /**
     * checkOffsetParams is a shorthand for the combined verification calls
     * required when using an API based on the (in, offset, len) format.
     *
     * @param dataLen
     * @param offset
     * @param len
     */
    public static void checkOffsetParams(final int dataLen,
                                         final int offset,
                                         final int len)
            throws ConstraintViolationException {
        Stodium.checkSize(offset, 0, dataLen, "0", "dataLen");
        Stodium.checkSize(offset + len, 0, dataLen, "0", "dataLen");
        Stodium.checkPositive(len);
    }

    /**
     * checkPow2 checks whether the given integer src is a power of 2, and
     * throws an exception otherwise.
     * @param src
     * @param descr
     * @throws SecurityException
     */
    public static void checkPow2(final int src,
                                 @NonNull final String descr)
            throws ConstraintViolationException {
        if ((src > 0) && ((src & (~src + 1)) == src)) {
            return;
        }
        throw new ConstraintViolationException(
                String.format("checkPow2 failed on [%s: %d]", descr, src));
    }

    /**
     *
     * @param src
     * @param descr
     * @throws ConstraintViolationException
     */
    public static void checkPow2(final long src,
                                 @NonNull final String descr)
            throws ConstraintViolationException {
        if ((src > 0) && ((src & (~src + 1)) == src)) {
            return;
        }
        throw new ConstraintViolationException(
                String.format("checkPow2 failed on [%s: %d]", descr, src));
    }

    /**
     * isEqual implements a Java-implementation of constant-time,
     * length-independent equality checking for sensitive values.
     *
     * @return true iff a == b
     */
    public static boolean isEqual(@NonNull final byte[] a,
                                  @NonNull final byte[] b) {
        if (a.length != b.length) {
            return false;
        }
        int result = 0;
        for (int i = 0; i < a.length; i++) {
            result |= a[i] ^ b[i];
        }
        return result == 0;
    }

    public static void wipeBytes(@NonNull final byte[] a) {
        Arrays.fill(a, (byte) 0x00);
    }

    /**
     *
     */
    @NonNull
    private static final AtomicBoolean initialized = new AtomicBoolean(false);

    /**
     * runInit wraps a call to sodium_init().
     */
    private void runInit()
            throws RuntimeException {
        if (initialized.get()) {
            return;
        }
        if (Sodium.sodium_init() == -1) {
            throw new RuntimeException("StodiumInit: could not initialize Sodium library");
        }
        initialized.set(true);
    }

    /**
     * Load the native library
     */
    static {
        System.loadLibrary("kaliumjni");
    }

    /**
     * Stodium constructor should be called once per application, or at least
     * before any class is used that requires the native methods to be
     * available. This ensures the library is loaded and initialized.
     */
    public static void StodiumInit() {
        new Stodium().runInit();
    }

    /**
     * SodiumVersionString returns the value of sodium_version_string().
     *
     * @return libsodium's version string
     */
    public static String SodiumVersionString() {
        return Sodium.sodium_version_string();
    }
}

