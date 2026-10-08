# Google Login for the roleplay server

The launcher uses Android Credential Manager to obtain a Google ID token, adds a random nonce, and exchanges the token with `POST /auth/google/mobile` over HTTPS. The default API is `https://openmp-gm.vercel.app`; set `AUTH_API_BASE_URL` when building if the backend moves.

## Google Cloud configuration

`GOOGLE_WEB_CLIENT_ID` must be the **Web OAuth client ID** also configured as `GOOGLE_CLIENT_ID` (or `GOOGLE_WEB_CLIENT_ID`) on the GM backend. Supply it to Gradle as an environment variable or the `googleWebClientId` project property. The client ID is not a secret; **never include a client secret in the APK**.

Register an Android OAuth client in Google Cloud Auth Platform for this app:

- Package name: `com.samp.mobile`
- Release signing certificate SHA-1: `5A:DF:5B:74:85:07:3A:DF:56:A5:62:AB:41:4E:46:5F:DF:CE:5A:64`

For debug APKs, register the debug signing certificate separately. See the official [Google Android client authentication guide](https://developers.google.com/android/guides/client-auth) and [Credential Manager Sign in with Google guide](https://developer.android.com/identity/sign-in/credential-manager-siwg).

## Registration profile

Google-authenticated new accounts must complete the UCP and character profile. All displayed fields are required: UCP account name for a new account, character name, country of birth (select from the 249-country list), birth date, gender, height, and weight. Height is entered in centimeters (80–250); weight is entered in kilograms (20–300). The backend validates and stores the entered height and weight rather than substituting defaults.

## Persistent sign-in and server access

The Google button is in **Settings**. After a successful sign-in, the launcher remembers the signed-in state and character name; the short-lived `AUTH…` game ticket remains single-use and is consumed on connection. If a ticket has expired, the launcher attempts to refresh the Google credential automatically. Use **Logout** on the Settings button to clear the launcher session and Credential Manager state.

The only listed server is `142.132.203.47:10125`. Custom server entry and the Add Server action are disabled. The launcher pins these values in its client configuration when connecting.

## Build a release

```bash
export GOOGLE_WEB_CLIENT_ID="<Web OAuth client ID>"
export AUTH_API_BASE_URL="https://openmp-gm.vercel.app"
./gradlew :app:assembleRelease
```

Release builds fail if `GOOGLE_WEB_CLIENT_ID` is empty, preventing an APK with a nonfunctional login button.
