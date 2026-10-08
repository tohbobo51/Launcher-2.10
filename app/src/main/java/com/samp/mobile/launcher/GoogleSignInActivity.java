package com.samp.mobile.launcher;

import android.app.DatePickerDialog;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Base64;
import android.text.InputFilter;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.credentials.Credential;
import androidx.credentials.CredentialManager;
import androidx.credentials.CredentialManagerCallback;
import androidx.credentials.CustomCredential;
import androidx.credentials.GetCredentialRequest;
import androidx.credentials.GetCredentialResponse;
import androidx.credentials.exceptions.GetCredentialException;

import com.google.android.libraries.identity.googleid.GetGoogleIdOption;
import com.google.android.libraries.identity.googleid.GetSignInWithGoogleOption;
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential;
import com.samp.mobile.BuildConfig;
import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;
import com.samp.mobile.launcher.util.GoogleAuthTicketStore;
import com.samp.mobile.launcher.util.NativeGoogleAuthApi;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import java.io.IOException;
import java.security.SecureRandom;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Locale;
import java.util.regex.Pattern;

public final class GoogleSignInActivity extends AppCompatActivity {
    public static final String EXTRA_LOGIN_NAME = "google_login_name";
    public static final String EXTRA_TICKET_EXPIRES_AT = "google_ticket_expires_at";
    public static final String EXTRA_AUTO_CONNECT = "google_auto_connect";

    private static final Pattern UCP_NAME_PATTERN =
            Pattern.compile("^[A-Za-z][A-Za-z0-9_]{1,29}[A-Za-z0-9]$");
    private static final Pattern CHARACTER_NAME_PATTERN =
            Pattern.compile("^[A-Za-z]{2,12}_[A-Za-z]{2,12}$");
    private static final Pattern BIRTHPLACE_PATTERN =
            Pattern.compile("^[A-Za-z][A-Za-z .,'-]{1,62}$");

    private boolean completed;
    private boolean autoConnectFlow;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        autoConnectFlow = getIntent().getBooleanExtra(EXTRA_AUTO_CONNECT, false);
        if (getSupportActionBar() != null) getSupportActionBar().hide();
        // Use an already authorized Google account first when reconnecting from the server tab.
        beginSignIn(!autoConnectFlow);
    }

    private void beginSignIn(boolean interactive) {
        String webClientId = BuildConfig.GOOGLE_WEB_CLIENT_ID;
        if (webClientId == null || webClientId.trim().isEmpty()) {
            finishWithError("Login Google belum dikonfigurasi untuk aplikasi ini.");
            return;
        }

        try {
            byte[] nonceBytes = new byte[32];
            new SecureRandom().nextBytes(nonceBytes);
            String nonce = Base64.encodeToString(
                    nonceBytes,
                    Base64.URL_SAFE | Base64.NO_WRAP | Base64.NO_PADDING
            );

            GetCredentialRequest.Builder requestBuilder = new GetCredentialRequest.Builder();
            if (interactive) {
                GetSignInWithGoogleOption googleOption = new GetSignInWithGoogleOption.Builder(
                        webClientId.trim()
                ).setNonce(nonce).build();
                requestBuilder.addCredentialOption(googleOption);
            } else {
                GetGoogleIdOption googleOption = new GetGoogleIdOption.Builder()
                        .setServerClientId(webClientId.trim())
                        .setFilterByAuthorizedAccounts(true)
                        .setAutoSelectEnabled(true)
                        .setNonce(nonce)
                        .build();
                requestBuilder.addCredentialOption(googleOption);
            }

            CredentialManager.create(this).getCredentialAsync(
                    this,
                    requestBuilder.build(),
                    null,
                    ContextCompat.getMainExecutor(this),
                    new CredentialManagerCallback<GetCredentialResponse, GetCredentialException>() {
                        @Override
                        public void onResult(GetCredentialResponse response) {
                            Credential credential = response.getCredential();
                            if (!(credential instanceof CustomCredential)
                                    || !GoogleIdTokenCredential.TYPE_GOOGLE_ID_TOKEN_CREDENTIAL
                                    .equals(credential.getType())) {
                                finishWithError("Google tidak mengembalikan kredensial login yang valid.");
                                return;
                            }
                            try {
                                GoogleIdTokenCredential googleCredential =
                                        GoogleIdTokenCredential.createFrom(
                                                ((CustomCredential) credential).getData());
                                exchangeGoogleToken(googleCredential.getIdToken(), nonce);
                            } catch (RuntimeException e) {
                                finishWithError("Tidak dapat membaca hasil login Google. Coba lagi.");
                            }
                        }

                        @Override
                        public void onError(GetCredentialException error) {
                            if (!interactive && !isFinishing() && !completed) {
                                // No cached consent/account: fall back to Google's interactive picker.
                                beginSignIn(true);
                            } else {
                                finishWithError("Login Google dibatalkan atau gagal. Silakan coba lagi.");
                            }
                        }
                    }
            );
        } catch (RuntimeException e) {
            finishWithError("Login Google tidak dapat dimulai. Silakan coba lagi.");
        }
    }

    private void exchangeGoogleToken(String idToken, String nonce) {
        Toast.makeText(this, "Memverifikasi akun Google…", Toast.LENGTH_SHORT).show();
        new Thread(() -> {
            try {
                NativeGoogleAuthApi.LoginTicket ticket =
                        NativeGoogleAuthApi.exchangeIdToken(idToken, nonce);
                runOnUiThread(() -> finishWithTicket(ticket));
            } catch (NativeGoogleAuthApi.AuthException e) {
                runOnUiThread(() -> {
                    if (isFinishing() || isDestroyed() || completed) return;
                    if ("REGISTRATION_REQUIRED".equals(e.code)) {
                        showRegistrationForm(idToken, nonce, true);
                    } else if ("CHARACTER_REGISTRATION_REQUIRED".equals(e.code)) {
                        showRegistrationForm(idToken, nonce, false);
                    } else {
                        finishWithError(e.getMessage());
                    }
                });
            } catch (IOException e) {
                runOnUiThread(() -> finishWithError(
                        "Tidak dapat menghubungi layanan login. Periksa koneksi lalu coba lagi."));
            }
        }, "xyron-google-auth").start();
    }

    private void showRegistrationForm(String idToken, String nonce, boolean createAccount) {
        if (isFinishing() || isDestroyed() || completed) return;
        setContentView(R.layout.activity_google_registration);

        TextView title = findViewById(R.id.registration_title);
        TextView subtitle = findViewById(R.id.registration_subtitle);
        TextInputLayout ucpLayout = findViewById(R.id.registration_ucp_layout);
        TextInputEditText ucpName = findViewById(R.id.registration_ucp);
        TextInputLayout characterLayout = findViewById(R.id.registration_character_layout);
        TextInputEditText characterName = findViewById(R.id.registration_character);
        TextInputLayout countryLayout = findViewById(R.id.registration_country_layout);
        AutoCompleteTextView country = findViewById(R.id.registration_country);
        TextInputLayout genderLayout = findViewById(R.id.registration_gender_layout);
        AutoCompleteTextView gender = findViewById(R.id.registration_gender);
        TextInputLayout heightLayout = findViewById(R.id.registration_height_layout);
        TextInputEditText height = findViewById(R.id.registration_height);
        TextInputLayout weightLayout = findViewById(R.id.registration_weight_layout);
        TextInputEditText weight = findViewById(R.id.registration_weight);
        MaterialButton birthdateButton = findViewById(R.id.registration_birthdate);
        MaterialButton submit = findViewById(R.id.registration_submit);
        MaterialButton cancel = findViewById(R.id.registration_cancel);

        if (createAccount) {
            title.setText("Create your account");
            subtitle.setText("Buat akun UCP dan profil karakter. Semua kolom wajib diisi; BB dalam kg dan TB dalam cm.");
        } else {
            title.setText("Create your character");
            subtitle.setText("Lengkapi profil karakter untuk akun ini. Semua kolom wajib diisi; BB dalam kg dan TB dalam cm.");
            ucpLayout.setVisibility(View.GONE);
        }
        ucpName.setFilters(new InputFilter[]{new InputFilter.LengthFilter(31)});
        characterName.setFilters(new InputFilter[]{new InputFilter.LengthFilter(23)});
        height.setFilters(new InputFilter[]{new InputFilter.LengthFilter(3)});
        weight.setFilters(new InputFilter[]{new InputFilter.LengthFilter(3)});

        String[] countries = getResources().getStringArray(R.array.registration_countries);
        ArrayAdapter<String> countryAdapter = new ArrayAdapter<>(
                this, android.R.layout.simple_dropdown_item_1line, countries);
        country.setAdapter(countryAdapter);
        country.setThreshold(0);
        country.setDropDownBackgroundDrawable(new ColorDrawable(Color.WHITE));
        country.setOnClickListener(view -> country.showDropDown());
        country.setOnFocusChangeListener((view, hasFocus) -> {
            if (hasFocus) country.showDropDown();
        });

        String[] genders = {"Male", "Female"};
        ArrayAdapter<String> genderAdapter = new ArrayAdapter<>(
                this, android.R.layout.simple_dropdown_item_1line, genders);
        gender.setAdapter(genderAdapter);
        gender.setOnClickListener(view -> gender.showDropDown());
        gender.setOnFocusChangeListener((view, hasFocus) -> {
            if (hasFocus) gender.showDropDown();
        });

        final String[] birthdate = {""};
        birthdateButton.setOnClickListener(view -> {
            Calendar today = Calendar.getInstance();
            Calendar initial = (Calendar) today.clone();
            initial.add(Calendar.YEAR, -18);
            DatePickerDialog picker = new DatePickerDialog(
                    this,
                    (datePicker, year, month, day) -> {
                        birthdate[0] = String.format(Locale.US, "%04d-%02d-%02d",
                                year, month + 1, day);
                        birthdateButton.setText(birthdate[0]);
                    },
                    initial.get(Calendar.YEAR), initial.get(Calendar.MONTH), initial.get(Calendar.DAY_OF_MONTH));
            picker.getDatePicker().setMaxDate(System.currentTimeMillis());
            picker.show();
        });
        cancel.setOnClickListener(view -> finishWithError(null));
        submit.setOnClickListener(view -> {
            ucpLayout.setError(null);
            characterLayout.setError(null);
            countryLayout.setError(null);
            genderLayout.setError(null);
            heightLayout.setError(null);
            weightLayout.setError(null);

            String ucpValue = createAccount ? valueOf(ucpName) : null;
            String characterValue = valueOf(characterName);
            String countryValue = country.getText() == null ? "" : country.getText().toString().trim();
            String genderValue = gender.getText() == null ? "" : gender.getText().toString().trim();
            String heightText = valueOf(height);
            String weightText = valueOf(weight);
            boolean valid = true;

            if (createAccount && !UCP_NAME_PATTERN.matcher(ucpValue).matches()) {
                ucpLayout.setError("3–31 karakter; mulai dengan huruf, tanpa spasi.");
                valid = false;
            }
            if (!CHARACTER_NAME_PATTERN.matcher(characterValue).matches()
                    || characterValue.length() > 23) {
                characterLayout.setError("Gunakan format Nama_Belakang dengan huruf Latin (maks. 23 karakter).");
                valid = false;
            }
            if (!BIRTHPLACE_PATTERN.matcher(countryValue).matches()
                    || !Arrays.asList(countries).contains(countryValue)) {
                countryLayout.setError("Pilih negara dari daftar.");
                valid = false;
            }
            if (!"Male".equals(genderValue) && !"Female".equals(genderValue)) {
                genderLayout.setError("Pilih gender.");
                valid = false;
            }
            if (birthdate[0].isEmpty()) {
                Toast.makeText(this, "Pilih tanggal lahir karakter.", Toast.LENGTH_SHORT).show();
                valid = false;
            }

            int heightValue = parseNumber(heightText);
            int weightValue = parseNumber(weightText);
            if (heightValue < 80 || heightValue > 250) {
                heightLayout.setError("Masukkan TB antara 80–250 cm.");
                valid = false;
            }
            if (weightValue < 20 || weightValue > 300) {
                weightLayout.setError("Masukkan BB antara 20–300 kg.");
                valid = false;
            }
            if (!valid) return;

            NativeGoogleAuthApi.RegistrationData registration =
                    new NativeGoogleAuthApi.RegistrationData(ucpValue, characterValue,
                            countryValue, birthdate[0], genderValue, heightValue, weightValue);
            submit.setEnabled(false);
            submit.setText("Memproses…");
            new Thread(() -> {
                try {
                    NativeGoogleAuthApi.LoginTicket ticket =
                            NativeGoogleAuthApi.registerIdToken(idToken, nonce, registration);
                    runOnUiThread(() -> finishWithTicket(ticket));
                } catch (NativeGoogleAuthApi.AuthException e) {
                    runOnUiThread(() -> {
                        if (isFinishing() || isDestroyed() || completed) return;
                        submit.setEnabled(true);
                        submit.setText("Daftar & masuk");
                        if ("UCP_NAME_TAKEN".equals(e.code)) {
                            ucpLayout.setError(e.getMessage());
                        } else if ("CHARACTER_NAME_TAKEN".equals(e.code)) {
                            characterLayout.setError(e.getMessage());
                        } else {
                            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
                        }
                    });
                } catch (IOException e) {
                    runOnUiThread(() -> finishWithError(
                            "Tidak dapat memastikan hasil pendaftaran. Silakan login Google lagi."));
                }
            }, "xyron-google-register").start();
        });
    }

    private String valueOf(TextInputEditText input) {
        return input.getText() == null ? "" : input.getText().toString().trim();
    }

    private int parseNumber(String value) {
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException ignored) {
            return -1;
        }
    }

    private void finishWithTicket(NativeGoogleAuthApi.LoginTicket ticket) {
        if (ticket == null || isFinishing() || isDestroyed() || completed) return;
        completed = true;
        GoogleAuthTicketStore.markSignedIn(this, ticket.characterName);

        if (autoConnectFlow) {
            Intent gameIntent = new Intent(this, SAMP.class);
            gameIntent.putExtra(SAMP.EXTRA_GOOGLE_LOGIN_TICKET, ticket.loginName);
            gameIntent.putExtra(SAMP.EXTRA_GOOGLE_LOGIN_EXPIRES_AT, ticket.expiresAtMillis);
            startActivity(gameIntent);
            finish();
            return;
        }

        Intent result = new Intent();
        result.putExtra(EXTRA_LOGIN_NAME, ticket.loginName);
        result.putExtra(EXTRA_TICKET_EXPIRES_AT, ticket.expiresAtMillis);
        setResult(RESULT_OK, result);
        finish();
    }

    private void finishWithError(String message) {
        if (completed || isFinishing()) return;
        completed = true;
        setResult(RESULT_CANCELED);
        if (message != null && !message.trim().isEmpty()) {
            Toast.makeText(this, message, Toast.LENGTH_LONG).show();
        }
        finish();
    }
}
