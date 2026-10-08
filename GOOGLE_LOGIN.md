# Login Google untuk server roleplay

Launcher memakai Android Credential Manager untuk mengambil Google ID token, membuat nonce acak, lalu menukarkan token ke endpoint `POST /auth/google/mobile`. API harus memakai HTTPS. Endpoint default adalah `https://openmp-gm.vercel.app`; ganti melalui `AUTH_API_BASE_URL` jika backend dipindahkan.

## OAuth client

`GOOGLE_WEB_CLIENT_ID` harus memakai **Web OAuth client ID** yang sama dengan `GOOGLE_CLIENT_ID` (atau `GOOGLE_WEB_CLIENT_ID`) pada backend GM. Berikan nilainya saat build melalui environment variable atau properti Gradle `googleWebClientId`. Client ID bukan rahasia; **jangan pernah menaruh client secret di APK**.

Selain Web client untuk validasi token di backend, daftarkan OAuth Android client pada Google Cloud Auth Platform untuk identitas APK ini:

- Package name: `com.samp.mobile`
- Release signing certificate SHA-1: `5A:DF:5B:74:85:07:3A:DF:56:A5:62:AB:41:4E:46:5F:DF:CE:5A:64`

Jika menguji APK debug, daftarkan juga package dan SHA-1 sertifikat debug yang digunakan. Lihat panduan resmi [Google client authentication](https://developers.google.com/android/guides/client-auth) dan [Credential Manager Sign in with Google](https://developer.android.com/identity/sign-in/credential-manager-siwg).

## Build release

```bash
export GOOGLE_WEB_CLIENT_ID="<Web OAuth client ID>"
export AUTH_API_BASE_URL="https://openmp-gm.vercel.app"
./gradlew :app:assembleRelease
```

Build release akan dihentikan jika `GOOGLE_WEB_CLIENT_ID` kosong, agar APK tidak diterbitkan dengan tombol login yang gagal karena konfigurasi tidak ada.

## Cara kerja ticket

Login Google dilakukan dari tab **Settings**. Setelah berhasil, launcher menyimpan ticket `AUTH…` satu kali pakai sampai waktu kedaluwarsa yang dikirim backend. Sambungkan server segera; opsi koneksi tanpa Google dan input nickname manual telah dihapus. Ticket dihapus dari penyimpanan aplikasi saat Connect, diteruskan ke native client untuk dibaca sekali, lalu dihapus dari file. GM menukar ticket dengan karakter yang sudah terdaftar. Akun Google baru tetap perlu menyelesaikan pendaftaran akun/karakter jika GM meminta data tersebut.
