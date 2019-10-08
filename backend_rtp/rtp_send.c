
int main() {
    struct voss_backend be;
    int channels = 2;
    int format = AFMT_S16_LE;
    
    rtp_open(&be, "sap.mcast.net",
        44100, 1024, &channels, &format);

    uint32_t buffer[1024];
    for (int i = 0; i < 1024; i++) {
        uint16_t left = sin(((float)i) / 1024.0f * M_PI * 440) * 16000;
        uint16_t right = cos(((float)i) / 1024.0f * M_PI * 220) * 16000;

        buffer[i] = left << 16 | right;
    }
    while (1) {
        rtp_play_transfer(&be, buffer, 1024);
    }
}
