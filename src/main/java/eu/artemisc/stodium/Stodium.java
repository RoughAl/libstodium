package eu.artemisc.stodium;

import android.support.annotation.NonNull;

import org.abstractj.kalium.Sodium;

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
     * @throws SecurityException
     */
    public static void checkStatus(final int status)
            throws SecurityException {
        if (status == 0) {
            return;
        }
        throw new SecurityException(
                String.format("Stodium: operation returned non-zero status %d", status));
    }

    /**
     *
     * @param src
     * @param expected
     * @param constant
     * @throws SecurityException
     */
    public static void checkSize(final int src,
                                 final int expected,
                                 @NonNull final String constant)
            throws SecurityException {
        if (src == expected) {
            return;
        }
        throw new SecurityException(
                String.format("Check size failed on [%s] [expected: %d, real: %d]",
                        constant, expected, src));
    }

    public static void checkSize(final int src,
                                 final int lower,
                                 final int upper,
                                 @NonNull final String lowerC,
                                 @NonNull final String upperC)
            throws SecurityException {
        if (src <= upper && src >= lower) {
            return;
        }
        throw new SecurityException(
                String.format("CheckSize failed on bounds [%s, %s] [lower: %d, upper: %d, read: %d]",
                        lowerC, upperC, lower, upper, src));
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

    /**
     * runInit wraps a call to sodium_init().
     */
    private void runInit()
            throws RuntimeException {
        if (Sodium.sodium_init() == -1) {
            throw new RuntimeException("StodiumInit: could not initialize Sodium library");
        }
    }

    // Load the library.
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

