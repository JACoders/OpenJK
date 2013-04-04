pushd ext_data\npcs
dir /b *.npc > _console_npc_list_
popd

pushd ext_data\sabers
dir /b *.sab > _console_sab_list_
popd

pushd ext_data\vehicles
dir /b *.veh > _console_veh_list_
popd

pushd ext_data\vehicles\weapons
dir /b *.vwp > _console_vwp_list_
popd

pushd ext_data\siege\classes
dir /b *.scl > _console_scl_list_
popd

pushd ext_data\siege\teams
dir /b *.team > _console_team_list_
popd

pushd scripts
REM dir /b *.bot > _console_bot_list_
dir /b *.arena > _console_arena_list_
popd

REM pushd forcecfg\light
REM dir /b *.fcf > _console_fcf_list_
REM popd

REM pushd forcecfg\dark
REM dir /b *.fcf > _console_fcf_list_
REM popd

pushd models\players
dir /b /a:d > _console_dir_list_
for /D %%M in (*) do (pushd %%M & dir /b *.skin > _console_skin_list_ & popd)
popd

pushd shaders
dir /b *.shader > _console_shader_list_
popd

pushd strings
dir /b /a:d > _console_dir_list_
for /D %%L in (*) do (pushd %%L & dir /b *.str > _console_str_list_ & popd)
popd
