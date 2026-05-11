# H01-uxms_unit

Captured-output harness for
[`missions/uxms_missions/uxms_unit`](../../../missions/uxms_missions/uxms_unit).
It launches a controlled MOOSDB publisher and runs bounded `uXMS` terminal
captures to verify scoping, column display, history, truncation, source filters,
and mission-file configuration behavior.

## Current Matrix

- `scoped_var_pass` Scopes one named variable and expects both the variable name and posted value to appear.
- `scoped_excludes_noise_pass` Scopes `XMS_ALPHA` and expects unrelated `XMS_NOISE` mail to remain absent from the display.
- `all_mode_pass` Uses `--all` and expects broad DB content, including `DB_UPTIME` and the scripted variable.
- `all_shortcut_pass` Uses the `-a` shortcut and expects broad DB content, including `DB_UPTIME` and the scripted variable.
- `show_source_pass` Enables the source column and expects `uTimerScript` to be visible as the publisher.
- `show_time_only_pass` Enables only the time column and expects the time header plus the scoped variable to render.
- `show_community_only_pass` Enables only the community column and expects the local `shoreside` community to be shown.
- `show_time_community_pass` Enables time and community columns and expects the local `shoreside` community to be shown.
- `show_source_community_pass` Enables source and community columns and expects both `uTimerScript` and `shoreside` evidence.
- `history_var_pass` Uses `--history=XMS_HIST` and expects the latest scripted history value to be visible.
- `trunc_long_payload_pass` Uses `--trunc=25` and expects the long payload to be clipped rather than printed in full.
- `clean_cli_scope_pass` Uses `--clean` with a mission file and expects CLI scope variables to override configured `var=` entries.
- `config_var_pass` Uses the mission-file `ProcessConfig = uXMS` block and expects configured variables to scope without CLI var names.
- `filter_prefix_pass` Uses `--filter=XMS_ --all` and expects XMS variables while suppressing unrelated DB variables.
- `src_timer_pass` Uses `--src=uTimerScript` and expects variables posted by that source to appear.
- `novirgins_pass` Uses `--novirgins` and expects an unposted scoped variable to stay hidden while posted mail remains visible.
- `novirgins_shortcut_pass` Uses the `-g` shortcut and expects an unposted scoped variable to stay hidden.
- `shortcut_source_pass` Uses the `-s` shortcut and expects source information to appear.
- `shortcut_source_time_pass` Uses the `-st` shortcut and expects source and time-oriented display to remain usable.
- `filter_specific_pass` Filters all DB output to `XMS_A` and expects `XMS_ALPHA` while suppressing other XMS variables.
- `filter_no_match_absent_pass` Uses a filter prefix that matches no DB variables and expects ordinary variables to stay hidden.
- `colorany_pass` Exercises `--colorany` while still expecting the scoped variable and value to render.
- `events_mode_pass` Uses event-driven refresh mode and expects a changed scoped variable to trigger output.
- `paused_shortcut_pass` Uses the `-p` shortcut and expects paused-mode startup output to include the scoped variable.
- `shortcut_trunc_pass` Uses the `-t` truncation shortcut and expects the long payload to be clipped.
- `alll_mode_pass` Uses `--alll` and expects broad DB content to remain visible.
- `server_underscore_args_pass` Connects with `--server_host` and `--server_port` aliases and expects scoped output.
- `moos_alias_args_pass` Connects with `--mooshost` and `--moosport` aliases and expects scoped output.
- `moos_underscore_args_pass` Connects with `--moos_host` and `--moos_port` aliases and expects scoped output.
- `alias_proc_name_pass` Runs uXMS with a custom process alias while still scoping the requested variable.
- `colormap_pair_pass` Parses a specific `--colormap` pair while still rendering the scoped variable.
- `clean_shortcut_scope_pass` Uses the short `-c` clean option and expects CLI scope variables to override mission-file variables.

## Run

```sh
./zlaunch.sh --jobs=4 --port_base=15200 --max_time=30 10
```

Run one inspectable case:

```sh
./zlaunch.sh --case=history_var_pass --port_base=15200 --max_time=30 10
```
