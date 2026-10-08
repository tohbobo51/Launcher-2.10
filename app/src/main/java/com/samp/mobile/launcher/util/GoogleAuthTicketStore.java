package com.samp.mobile.launcher.util;

import android.content.Context;
import android.content.SharedPreferences;

/** Stores persistent account status and a short-lived, single-use game ticket. */
public final class GoogleAuthTicketStore {
    private static final String PREFS = "google_login_session";
    private static final String KEY_TICKET = "ticket";
    private static final String KEY_EXPIRES_AT = "expires_at";
    private static final String KEY_SIGNED_IN = "signed_in";
    private static final String KEY_CHARACTER_NAME = "character_name";

    private GoogleAuthTicketStore() {
    }

    public static boolean save(Context context, String ticket, long expiresAtMillis) {
        if (context == null || ticket == null
                || !ticket.matches("AUTH[A-HJ-NP-Z2-9]{16}")
                || expiresAtMillis <= System.currentTimeMillis()) {
            clearTicket(context);
            return false;
        }
        preferences(context).edit()
                .putString(KEY_TICKET, ticket)
                .putLong(KEY_EXPIRES_AT, expiresAtMillis)
                .apply();
        return true;
    }

    public static void markSignedIn(Context context, String characterName) {
        if (context == null) return;
        preferences(context).edit()
                .putBoolean(KEY_SIGNED_IN, true)
                .putString(KEY_CHARACTER_NAME, characterName == null ? "" : characterName)
                .apply();
    }

    public static boolean isSignedIn(Context context) {
        return context != null && preferences(context).getBoolean(KEY_SIGNED_IN, false);
    }

    public static String getCharacterName(Context context) {
        return context == null ? "" : preferences(context).getString(KEY_CHARACTER_NAME, "");
    }

    public static boolean hasValidTicket(Context context) {
        if (context == null) return false;
        SharedPreferences prefs = preferences(context);
        String ticket = prefs.getString(KEY_TICKET, null);
        long expiresAt = prefs.getLong(KEY_EXPIRES_AT, 0L);
        if (isValid(ticket, expiresAt)) return true;
        clearTicket(context);
        return false;
    }

    /** Returns the active ticket and removes it from preferences so it can be used only once. */
    public static Ticket consume(Context context) {
        if (context == null) return null;
        SharedPreferences prefs = preferences(context);
        String loginName = prefs.getString(KEY_TICKET, null);
        long expiresAt = prefs.getLong(KEY_EXPIRES_AT, 0L);
        prefs.edit().remove(KEY_TICKET).remove(KEY_EXPIRES_AT).apply();
        if (!isValid(loginName, expiresAt)) return null;
        return new Ticket(loginName, expiresAt);
    }

    /** Clears only the one-use game ticket; the persistent Google account remains signed in. */
    public static void clear(Context context) {
        clearTicket(context);
    }

    /** Logs out of this launcher and removes both the account marker and any pending ticket. */
    public static void logout(Context context) {
        if (context != null) preferences(context).edit().clear().apply();
    }

    private static void clearTicket(Context context) {
        if (context != null) {
            preferences(context).edit().remove(KEY_TICKET).remove(KEY_EXPIRES_AT).apply();
        }
    }

    private static SharedPreferences preferences(Context context) {
        return context.getApplicationContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    private static boolean isValid(String ticket, long expiresAtMillis) {
        return ticket != null
                && ticket.matches("AUTH[A-HJ-NP-Z2-9]{16}")
                && expiresAtMillis > System.currentTimeMillis();
    }

    public static final class Ticket {
        public final String loginName;
        public final long expiresAtMillis;

        private Ticket(String loginName, long expiresAtMillis) {
            this.loginName = loginName;
            this.expiresAtMillis = expiresAtMillis;
        }
    }
}
