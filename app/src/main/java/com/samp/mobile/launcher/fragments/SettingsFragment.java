package com.samp.mobile.launcher.fragments;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.content.ContextCompat;
import androidx.credentials.ClearCredentialStateRequest;
import androidx.credentials.CredentialManager;
import androidx.credentials.CredentialManagerCallback;
import androidx.credentials.exceptions.ClearCredentialException;
import androidx.fragment.app.Fragment;

import com.joom.paranoid.Obfuscate;
import com.samp.mobile.R;
import com.samp.mobile.launcher.MainActivity;
import com.samp.mobile.launcher.GoogleSignInActivity;
import com.samp.mobile.launcher.SplashActivity;
import com.samp.mobile.launcher.util.ButtonAnimator;
import com.samp.mobile.launcher.util.GoogleAuthTicketStore;
import com.samp.mobile.launcher.util.SharedPreferenceCore;
import com.samp.mobile.launcher.util.Util;

import org.ini4j.InvalidFileFormatException;
import org.ini4j.Wini;

import java.io.File;
import java.io.IOException;
@Obfuscate
public class SettingsFragment extends Fragment {

    private static final int REQUEST_GOOGLE_LOGIN = 0x4751;

    Wini mWini = null;
    Button mGoogleLoginButton;
    SwitchCompat mKeyboardSwitch;
    SwitchCompat mVoiceSwitch;
    SwitchCompat mModifySwitch;
    SwitchCompat mFPSSwitch;
    SwitchCompat mMonetSwitch;
    SeekBar mMessagesSeekBar;
    TextView mMessagesText;
    SeekBar mFPSSeekBar;
    TextView mFPSText;

    String[] titles = {"0.3.7", "0.3.7-R1", "0.3.7-R3","0.3.7-R4","0.3.7-R5"};

    Spinner autoCompleteTextView;
    ArrayAdapter<String> adapter;


    @Override
    public View onCreateView(LayoutInflater layoutInflater, ViewGroup viewGroup, Bundle bundle) {
        View view = layoutInflater.inflate(R.layout.fragment_settings, viewGroup, false);

        ((MainActivity)getActivity()).hideKeyboard(getActivity());

        mGoogleLoginButton = view.findViewById(R.id.google_login_button);
        mGoogleLoginButton.setOnTouchListener(new ButtonAnimator(getContext(), mGoogleLoginButton));
        mGoogleLoginButton.setOnClickListener(v -> onGoogleButtonClicked());
        refreshGoogleLoginButton();
        autoCompleteTextView = view.findViewById(R.id.spinner);
        mKeyboardSwitch = view.findViewById(R.id.keyboard_switch);
        mFPSSwitch = view.findViewById(R.id.fps_switch);
        mMonetSwitch = view.findViewById(R.id.monet_switch);
        mVoiceSwitch = view.findViewById(R.id.voice_switch);
        mModifySwitch = view.findViewById(R.id.modify_switch);
        mMessagesSeekBar = view.findViewById(R.id.messages_seekbar);
        mMessagesText = view.findViewById(R.id.messages_count);
        mFPSSeekBar = view.findViewById(R.id.fps_seekbar);
        mFPSText = view.findViewById(R.id.fps_count);

        adapter = new ArrayAdapter<String>(getActivity(), androidx.appcompat.R.layout.support_simple_spinner_dropdown_item, titles);
        adapter.setDropDownViewResource(androidx.appcompat.R.layout.support_simple_spinner_dropdown_item);
        autoCompleteTextView.setAdapter(adapter);

        File file = new File(getActivity().getExternalFilesDir(null) + "/SAMP/settings.ini");
        try {
            mWini = new Wini(file);
        } catch (IOException e) {
            e.printStackTrace();
        }

        autoCompleteTextView.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                new SharedPreferenceCore().setInt(requireContext().getApplicationContext(), "VERSION", position);
                File file = new File(getActivity().getExternalFilesDir(null) + "/SAMP/settings.ini");
                if(file.exists()) {
                    try {
                        if(mWini != null) {
                            mWini.put("client", "version", titles[new SharedPreferenceCore().getInt(requireContext().getApplicationContext(), "VERSION")]);
                            mWini.store();
                        }
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        mModifySwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                new SharedPreferenceCore().setBoolean(requireContext().getApplicationContext(), "MODIFIED_DATA", b);
            }
        });

        mKeyboardSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                new SharedPreferenceCore().setBoolean(requireContext().getApplicationContext(), "ANDROID_KEYBOARD", b);
                try {
                    if(mWini != null) {
                        mWini.put("gui", "androidkeyboard", b);
                        mWini.store();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });

        mVoiceSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                new SharedPreferenceCore().setBoolean(requireContext().getApplicationContext(), "AIM", b);
                try {
                    if(mWini != null) {
                        mWini.put("gui", "autoaim", b);
                        mWini.store();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });

        mFPSSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                new SharedPreferenceCore().setBoolean(requireContext().getApplicationContext(), "FPS_DISPLAY", b);
                try {
                    if(mWini != null) {
                        mWini.put("gui", "fps", b ? 1 : 0);
                        mWini.store();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });

        mMonetSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                new SharedPreferenceCore().setBoolean(requireContext().getApplicationContext(), "MLOADER", b);
            }
        });

        // perform seek bar change listener event used for getting the progress value
        mMessagesSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int realProgress = 0;
                switch(progress)
                {
                    case 0: {
                        realProgress = 6;
                        break;
                    }
                    case 1: {
                        realProgress = 9;
                        break;
                    }
                    case 2: {
                        realProgress = 12;
                        break;
                    }
                    case 3:{
                        realProgress = 15;
                        break;
                    }
                }
                new SharedPreferenceCore().setInt(requireContext().getApplicationContext(), "MESSAGE_COUNT", realProgress);
                File file = new File(getActivity().getExternalFilesDir(null) + "/SAMP/settings.ini");
                if(file.exists()) {
                    try {
                        if(mWini != null) {
                            mWini.put("gui", "ChatMaxMessages", realProgress);
                            mWini.store();
                        }
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                mMessagesText.setText(String.valueOf(realProgress));
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        // perform seek bar change listener event used for getting the progress value
        mFPSSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int realProgress = 0;
                switch(progress)
                {
                    case 0: {
                        realProgress = 30;
                        break;
                    }
                    case 1: {
                        realProgress = 60;
                        break;
                    }
                    case 2: {
                        realProgress = 90;
                        break;
                    }
                    case 3:{
                        realProgress = 120;
                        break;
                    }
                }
                new SharedPreferenceCore().setInt(requireContext().getApplicationContext(), "FPS_LIMIT", realProgress);
                File file = new File(getActivity().getExternalFilesDir(null) + "/SAMP/settings.ini");
                if(file.exists()) {
                    try {
                        if(mWini != null) {
                            mWini.put("gui", "FPSLimit", realProgress);
                            mWini.store();
                        }
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                mFPSText.setText(String.valueOf(realProgress));
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });


        return view;
    }

    private void refreshGoogleLoginButton() {
        if (mGoogleLoginButton == null || getContext() == null) {
            return;
        }
        boolean signedIn = GoogleAuthTicketStore.isSignedIn(requireContext());
        mGoogleLoginButton.setText(signedIn ? "Google aktif  ·  Logout" : "Login dengan Google");
        mGoogleLoginButton.setContentDescription(signedIn
                ? "Google terhubung. Ketuk untuk logout."
                : "Login dengan Google");
    }

    private void onGoogleButtonClicked() {
        if (GoogleAuthTicketStore.isSignedIn(requireContext())) {
            String characterName = GoogleAuthTicketStore.getCharacterName(requireContext());
            String message = characterName == null || characterName.isEmpty()
                    ? "Keluar dari akun Google launcher ini?"
                    : "Keluar dari akun Google untuk " + characterName + "?";
            new AlertDialog.Builder(requireContext())
                    .setTitle("Logout Google")
                    .setMessage(message)
                    .setNegativeButton("Batal", null)
                    .setPositiveButton("Logout", (dialog, which) -> logoutGoogle())
                    .show();
            return;
        }
        startActivityForResult(new Intent(requireContext(), GoogleSignInActivity.class),
                REQUEST_GOOGLE_LOGIN);
    }

    private void logoutGoogle() {
        GoogleAuthTicketStore.logout(requireContext());
        refreshGoogleLoginButton();
        Toast.makeText(requireContext(), "Akun Google sudah logout dari launcher.",
                Toast.LENGTH_SHORT).show();
        try {
            CredentialManager.create(requireContext()).clearCredentialStateAsync(
                    new ClearCredentialStateRequest(),
                    null,
                    ContextCompat.getMainExecutor(requireContext()),
                    new CredentialManagerCallback<Void, ClearCredentialException>() {
                        @Override
                        public void onResult(Void result) {
                            // Credential provider session is cleared as well as the local marker.
                        }

                        @Override
                        public void onError(ClearCredentialException error) {
                            Log.w("GoogleAuth", "Could not clear provider credential state", error);
                        }
                    });
        } catch (RuntimeException error) {
            Log.w("GoogleAuth", "Could not clear provider credential state", error);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_GOOGLE_LOGIN) {
            return;
        }
        if (resultCode != Activity.RESULT_OK || data == null) {
            refreshGoogleLoginButton();
            return;
        }

        String loginName = data.getStringExtra(GoogleSignInActivity.EXTRA_LOGIN_NAME);
        long expiresAt = data.getLongExtra(GoogleSignInActivity.EXTRA_TICKET_EXPIRES_AT, 0L);
        if (!GoogleAuthTicketStore.save(requireContext(), loginName, expiresAt)) {
            Toast.makeText(requireContext(), "Sesi Google tidak valid atau sudah kedaluwarsa. Login lagi.",
                    Toast.LENGTH_LONG).show();
            refreshGoogleLoginButton();
            return;
        }

        refreshGoogleLoginButton();
        Toast.makeText(requireContext(), "Google berhasil login. Sambungkan server sekarang.",
                Toast.LENGTH_LONG).show();
    }

    @Override
    public void onResume() {
        super.onResume();

        mKeyboardSwitch.setChecked(new SharedPreferenceCore().getBoolean(requireContext().getApplicationContext(), "ANDROID_KEYBOARD"));
        mVoiceSwitch.setChecked(new SharedPreferenceCore().getBoolean(requireContext().getApplicationContext(), "AIM"));
        mFPSSwitch.setChecked(new SharedPreferenceCore().getBoolean(requireContext().getApplicationContext(), "FPS_DISPLAY"));
        mModifySwitch.setChecked(new SharedPreferenceCore().getBoolean(requireContext().getApplicationContext(), "MODIFIED_DATA"));
        mMonetSwitch.setChecked(new SharedPreferenceCore().getBoolean(requireContext().getApplicationContext(), "MLOADER"));
        autoCompleteTextView.setSelection(new SharedPreferenceCore().getInt(requireContext().getApplicationContext(), "VERSION"));

        int fps = new SharedPreferenceCore().getInt(getContext(), "FPS_LIMIT");
        switch (fps)
        {
            case 30: mFPSSeekBar.setProgress(0); break;
            case 60: mFPSSeekBar.setProgress(1); break;
            case 90: mFPSSeekBar.setProgress(2); break;
            case 120: mFPSSeekBar.setProgress(3); break;
        }
        mFPSText.setText(String.valueOf(fps));

        int message = new SharedPreferenceCore().getInt(getContext(), "MESSAGE_COUNT");
        switch (message)
        {
            case 6: mMessagesSeekBar.setProgress(0); break;
            case 9: mMessagesSeekBar.setProgress(1); break;
            case 12: mMessagesSeekBar.setProgress(2); break;
            case 15: mMessagesSeekBar.setProgress(3); break;
        }
        mMessagesText.setText(String.valueOf(message));
        refreshGoogleLoginButton();
    }
}
