package com.samp.mobile.launcher.util;

import android.content.Context;
import android.content.SharedPreferences;

/** Holds a short-lived Google game ticket until the player connects to a server. */
public final class GoogleAuthTicketStore {
    private static final String PREFS = "google_login_session";
    private static final String KEY_TICKET = "ticket";
    private static final String KEY_EXPIRES_AT = "expires_at";

    private GoogleAuthTicketStore() {
    }

    public static boolean save(Context context, String ticket, long expiresAtMillis) {
        if (context == null || ticket == null
                || !ticket.matches("AUTH[A-HJ-NP-Z2-9]{16}")
                || expiresAtMillis <= System.currentTimeMillis()) {
            clear(context);
            return false;
        }
        preferences(context).edit()
                .putString(KEY_TICKET, ticket)
                .putLong(KEY_EXPIRES_AT, expiresAtMillis)
                .apply();
        return true;
    }

    public static boolean hasValidTicket(Context context) {
        if (context == null) {
            return false;
        }
        SharedPreferences preferences = preferences(context);
        String ticket = preferences.getString(KEY_TICKET, null);
        long expiresAt = preferences.getLong(KEY_EXPIRES_AT, 0L);
        if (isValid(ticket, expiresAt)) {
            return true;
        }
        clear(context);
        return false;
    }

    /** Returns the active ticket and removes it from preferences so it can be used only once. */
    public static Ticket consume(Context context) {
        if (context == null) {
            return null;
        }
        SharedPreferences preferences = preferences(context);
        String loginName = preferences.getString(KEY_TICKET, null);
        long expiresAt = preferences.getLong(KEY_EXPIRES_AT, 0L);
        preferences.edit().remove(KEY_TICKET).remove(KEY_EXPIRES_AT).apply();
        if (!isValid(loginName, expiresAt)) {
            return null;
        }
        return new Ticket(loginName, expiresAt);
    }

    public static void clear(Context context) {
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
