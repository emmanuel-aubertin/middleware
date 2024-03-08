```cpp
        libvlc_media_player_t *mp;
        libvlc_media_t *m;

        const char *const vlc_args[] = {
            "--verbose=2"};

        libvlc_instance_t *inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

        if (inst == NULL) // TODO: fix, always true LibVLC never init
        {
            failure("LibVLC initialization failed.");
            return -1;
        }

        m = libvlc_media_new_path(inst, argv[1]);

        mp = libvlc_media_player_new_from_media(m);
        libvlc_media_release(m);
        libvlc_media_player_play(mp);

        sleep(10);

        libvlc_media_player_stop(mp);
        libvlc_media_player_release(mp);
        libvlc_release(inst);
```