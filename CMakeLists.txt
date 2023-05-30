project(crystalpicnic)
cmake_minimum_required(VERSION 2.6)

# Options

# Extras
option(BATTLED2 "Build battle level editor" off)
option(TOOLS "Build extra tools" off)

# Features
option(WELL512 "Use well512 RNG" off)

# Platforms
option(RASPBERRYPI "Build for Raspberry Pi" off)

# Compile options
option(ALLEGRO_MONOLITH "Use the monolith allegro library" on)
option(DEBUG "Build with debug symbols" off)
option(STEAMWORKS "Enable Steamworks" off)
option(LOAD_FROM_FILESYSTEM "Don't use CPAs" off)
option(TRANSLATION_BUILD "Build for working on translations" off)

# String options

set(ALLEGRO_SUFFIX "-static" CACHE STRING "Allegro library suffix")

set(USER_INCLUDE_PATH "" CACHE FILEPATH "Any extra include directories you want")
include_directories("${CMAKE_SOURCE_DIR}/include")
if(NOT "${USER_INCLUDE_PATH}" STREQUAL "")
	include_directories(${USER_INCLUDE_PATH})
endif()

set(USER_LIBRARY_PATH "" CACHE FILEPATH "Any extra library directories you want")
if(NOT "${USER_LIBRARY_PATH}" STREQUAL "")
	link_directories("${USER_LIBRARY_PATH}")
endif()

# Option configuration

if(WELL512)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DWELL512=1")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DWELL512=1")
endif()

if(RASPBERRYPI)
	set(CMAKE_C_FLAGS_RELEASE "-Wall -Os")
	set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Os")
	link_directories("/opt/vc/lib")
endif()

# Inputs

set(COMMON_SOURCES
	src/abilities.cpp
	src/animation.cpp
	src/animation_set.cpp
	src/area_loop.cpp
	src/area_manager.cpp
	src/astar.cpp
	src/astar_character_role.cpp
	src/atlas.cpp
	src/atlas_accessor.cpp
	src/audio_config_loop.cpp
	src/bass.cpp
	src/bass_fileprocs.cpp
	src/battle_ai.cpp
	src/battle_entity.cpp
	src/battle_loop.cpp
	src/battle_pathfind.cpp
	src/battle_transition_in.cpp
	src/bitmap.cpp
	src/bones.cpp
	src/camera.cpp
	src/character_map_entity.cpp
	src/character_role.cpp
	src/collidable.cpp
	src/collision_detection.cpp
	src/config.cpp
	src/cpa.cpp
	src/credits_loop.cpp
	src/crystal_loop.cpp
	src/difficulty_loop.cpp
	src/direct3d.cpp
	src/dirty.cpp
	src/enemy_avatar.cpp
	src/enemy_avatar_wander_character_role.cpp
	src/engine.cpp
	src/entity.cpp
	src/error.cpp
	src/follow_character_role.cpp
	src/frame.cpp
	src/game_specific_globals.cpp
	src/general.cpp
	src/graphics.cpp
	src/input_config_loop.cpp
	src/language_config_loop.cpp
	src/lua.cpp
	src/main.cpp
	src/main_menu_loop.cpp
	src/map_entity.cpp
	src/map_loop.cpp
	src/mt19937ar.c
	src/music.cpp
	src/npc.cpp
	src/particle.cpp
	src/player.cpp
	src/resource_manager.cpp
	src/runner_loop.cpp
	src/saveload_loop.cpp
	src/settings_loop.cpp
	src/shaders.cpp
	src/shop_loop.cpp
	src/skeleton.cpp
	src/snprintf.c
	src/sound.cpp
	src/speech_loop.cpp
	src/steering.cpp
	src/triangulate.cpp
	src/video_config_loop.cpp
	src/video_player.cpp
	src/wander_character_role.cpp
	src/weaponized_entity.cpp
	src/well512.c
	src/whack_a_skunk_loop.cpp
	src/widgets.cpp
	src/wrap.cpp
	src/xml.cpp
)

if(STEAMWORKS)
	LIST(APPEND COMMON_SOURCES src/steamworks.cpp)
endif()

set(EXE_SOURCES src/crystalpicnic.cpp)
LIST(APPEND EXE_SOURCES
	third_party/poly2tri/poly2tri/common/shapes.cc
	third_party/poly2tri/poly2tri/sweep/advancing_front.cc
	third_party/poly2tri/poly2tri/sweep/cdt.cc
	third_party/poly2tri/poly2tri/sweep/sweep.cc
	third_party/poly2tri/poly2tri/sweep/sweep_context.cc
)
if(WIN32)
	LIST(APPEND EXE_SOURCES icons/windows_icon.rc)
endif()

# Outputs

add_library(my_load_bitmap STATIC src/my_load_bitmap.cpp)
add_library(crystalpicnic-engine STATIC	${COMMON_SOURCES})
add_executable(CrystalPicnic ${EXE_SOURCES})

# Platform specifics

if(MINGW)
	set(FLAGS "${FLAGS} -DTGUI2_STATIC -DALLEGRO_STATICLINK -DALLEGRO_UNSTABLE")
	set(LFLAGS "${LFLAGS} -static -static-libgcc -static-libstdc++")
	set(LFLAGS_WIN32 "${LFLAGS} -Wl,--subsystem,windows")
	if(LOAD_FROM_FILESYSTEM)
		set(FLAGS "${FLAGS} -DLOAD_FROM_FILESYSTEM")
	endif()
	if(TRANSLATION_BUILD)
		set(FLAGS "${FLAGS} -DTRANSLATION_BUILD")
	endif()
	if(STEAMWORKS)
		set(FLAGS "${FLAGS} -DSTEAMWORKS")
	endif()
	set_target_properties(my_load_bitmap PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties(crystalpicnic-engine PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties(CrystalPicnic PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties(CrystalPicnic PROPERTIES LINK_FLAGS "${LFLAGS_WIN32}")
	set(ALLEGRO_MAIN_LIB "")
	set(EXTRA_LIBS "opengl32" "dxguid" "dinput8" "xinput" "dsound" "d3d9" "d3dx9" "kernel32" "user32" "gdi32" "comdlg32" "ole32" "winmm" "psapi" "gdiplus" "uuid" "shlwapi" "dbghelp")
	if(STEAMWORKS)
		LIST(APPEND EXTRA_LIBS "steam_api")
	endif()
	if(DEBUG)
		set(TGUI2_LIB_NAME "tgui2-debug-static")
	else()
		set(TGUI2_LIB_NAME "tgui2-static")
	endif()
	set(ALLEGRO_LIB "allegro_monolith-static")
	set(ZLIB_LIB_NAME "zlibstatic")
	set(LUA_LIB "lua5.2")
	set(FREETYPE_LIB "freetype")
	set(MAIN_LIBS
		crystalpicnic-engine
		my_load_bitmap
		${TGUI2_LIB_NAME}
		${ALLEGRO_LIB}
		${LUA_LIB}
		${FREETYPE_LIB}
		${ZLIB_LIB_NAME}
		bassmidi
		bass
		${EXTRA_LIBS}
	)
elseif(WIN32)
	SET(FLAGS "/D_CRT_SECURE_NO_WARNINGS /D_USE_MATH_DEFINES /DALLEGRO_STATICLINK /EHsc /D_WIN32_WINNT=0x501 /MP /DALLEGRO_UNSTABLE /DTGUI2_STATIC")
	if(DEBUG)
		set(FLAGS "${FLAGS} /MTd")
	else()
		set(FLAGS "${FLAGS} /MT")
	endif()
	if(LOAD_FROM_FILESYSTEM)
		set(FLAGS "${FLAGS} /DLOAD_FROM_FILESYSTEM")
	endif()
	if(TRANSLATION_BUILD)
		set(FLAGS "${FLAGS} /DTRANSLATION_BUILD")
	endif()
	if(STEAMWORKS)
		set(FLAGS "${FLAGS} /DSTEAMWORKS")
	endif()
	set_target_properties(my_load_bitmap PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties(crystalpicnic-engine PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties(CrystalPicnic PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties(CrystalPicnic PROPERTIES LINK_FLAGS "/SAFESEH:NO /SUBSYSTEM:WINDOWS")
	set(ALLEGRO_MAIN_LIB "")
	set(EXTRA_LIBS "opengl32" "dxguid" "dinput8" "xinput" "dsound" "d3d9" "kernel32" "user32" "gdi32" "comdlg32" "ole32" "winmm" "psapi" "gdiplus" "uuid" "shlwapi" "dbghelp")
	if(STEAMWORKS)
		LIST(APPEND EXTRA_LIBS "steam_api")
	endif()
	if(DEBUG)
		set(TGUI2_LIB_NAME "tgui2-debug-static")
	else()
		set(TGUI2_LIB_NAME "tgui2-static")
	endif()
	set(ALLEGRO_LIB "allegro_monolith-static")
	set(ZLIB_LIB_NAME "zlibstatic")
	set(LUA_LIB "lua5.2")
	set(FREETYPE_LIB "freetype")
	set(MAIN_LIBS
		crystalpicnic-engine
		my_load_bitmap
		${TGUI2_LIB_NAME}
		${ALLEGRO_LIB}
		${LUA_LIB}
		${FREETYPE_LIB}
		${ZLIB_LIB_NAME}
		bassmidi
		bass
		${EXTRA_LIBS}
		"c:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86\\d3dx9.lib"
	)
elseif(APPLE)
	list(APPEND COMMON_SOURCES src/apple.m)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wno-unknown-pragmas -Wno-comment")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unknown-pragmas -Wno-comment")
	if(LOAD_FROM_FILESYSTEM)
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DLOAD_FROM_FILESYSTEM=1")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLOAD_FROM_FILESYSTEM=1")
	endif()
	if(TRANSLATION_BUILD)
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DTRANSLATION_BUILD=1")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DTRANSLATION_BUILD=1")
	endif()
	set(ALLEGRO_MAIN_LIB "allegro_main${ALLEGRO_SUFFIX}")
	FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
	FIND_LIBRARY(FOUNDATION_LIBRARY Foundation)
	FIND_LIBRARY(APPSERVICES_LIBRARY ApplicationServices)
	FIND_LIBRARY(APPKIT_LIBRARY AppKit)
	FIND_LIBRARY(IOKIT_LIBRARY IOKit)
	FIND_LIBRARY(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	set(EXTRA_LIBS ${IOKIT_LIBRARY} ${OPENGL_LIBRARY} ${FOUNDATION_LIBRARY} ${APPSERVICES_LIBRARY} /Users/trent/code/local/lib/libfreetype.a ${APPKIT_LIBRARY} ${AUDIOTOOLBOX_LIBRARY} bass bassmidi tgui2 /Users/trent/code/local/lib/libz.a)
	set(MAIN_LIBS
		crystalpicnic-engine
		my_load_bitmap
		allegro_monolith${ALLEGRO_SUFFIX}
		/Users/trent/code/local/lib/liblua5.2.a
		${EXTRA_LIBS}
	)
else()
	# Linux

	if(LOAD_FROM_FILESYSTEM)
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DLOAD_FROM_FILESYSTEM=1")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLOAD_FROM_FILESYSTEM=1")
	endif()
	if(TRANSLATION_BUILD)
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DTRANSLATION_BUILD=1")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DTRANSLATION_BUILD=1")
	endif()
	
	if(RASPBERRYPI)
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unknown-pragmas -march=armv6 -mfpu=vfp -mfloat-abi=hard -DALLEGRO_UNSTABLE")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unknown-pragmas -march=armv6 -mfpu=vfp -mfloat-abi=hard -DALLEGRO_UNSTABLE")

		include_directories("/opt/vc/include")
		include_directories("/opt/vc/include/interface/vcos/pthreads")

		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,stuff")

		set(MAIN_LIBS
			crystalpicnic-engine
			my_load_bitmap
			tgui2-static
			allegro_monolith${ALLEGRO_SUFFIX}
			${ALLEGRO_MAIN_LIB}
			/home/pi/code/crystal-picnic/build/libbassmidi.so
			/home/pi/code/crystal-picnic/build/libbass.so
			lua
			freetype
			png
			z
			brcmGLESv2
			brcmEGL
			bcm_host
			X11
			pthread
		)
	else(RASPBERRYPI)
		# Desktop Linux

		if(DEBUG)
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -Wall -Wno-unknown-pragmas -Wno-comment -DALLEGRO_UNSTABLE")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -Wall -Wno-unknown-pragmas -Wno-comment -DALLEGRO_UNSTABLE -std=c++0x")
		else()
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -Wall -Wno-unknown-pragmas -Wno-comment -DALLEGRO_UNSTABLE")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -Wno-unknown-pragmas -Wno-comment -DALLEGRO_UNSTABLE -std=c++0x")
		endif()

		if(STEAMWORKS)
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DSTEAMWORKS=1")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DSTEAMWORKS=1")
			SET(STEAMWORKS_LIBRARY "${CMAKE_BINARY_DIR}/libsteam_api.so")
		else()
			SET(STEAMWORKS_LIBRARY "")
		endif()

		if(ALLEGRO_MONOLITH)
			set(ALLEGRO_LIBRARIES allegro_monolith${ALLEGRO_SUFFIX} ${ALLEGRO_MAIN_LIB})
		else()
			set(ALLEGRO_LIBRARIES
				allegro${ALLEGRO_SUFFIX}
				allegro_font${ALLEGRO_SUFFIX}
				allegro_image${ALLEGRO_SUFFIX}
				allegro_memfile${ALLEGRO_SUFFIX}
				allegro_primitives${ALLEGRO_SUFFIX}
				allegro_ttf${ALLEGRO_SUFFIX}
			)
		endif()

		if(STEAMWORKS)
			set(LUA_LIBRARY "/usr/local/lib/liblua.a")
			set(PNG_LIBRARY "png12")
			set(FREETYPE_LIBRARY "freetype")
			set(ZLIB_LIBRARY "z")
			set(BASS_LIBRARY "/home/trent/code/crystal-picnic/build/libbass.so")
			set(BASSMIDI_LIBRARY "/home/trent/code/crystal-picnic/build/libbassmidi.so")
			set(OTHER_LIBRARIES pthread dl)
		else()
			set(LUA_LIBRARY "/usr/lib/x86_64-linux-gnu/liblua5.2.a")
			set(PNG_LIBRARY "/usr/lib/x86_64-linux-gnu/libpng16.a")
			set(FREETYPE_LIBRARY "/usr/lib/x86_64-linux-gnu/libfreetype.a")
			set(ZLIB_LIBRARY "/usr/lib/x86_64-linux-gnu/libz.a")
			set(BROTLI_LIBRARY "/usr/lib/x86_64-linux-gnu/libbrotlidec.a" "/usr/lib/x86_64-linux-gnu/libbrotlicommon.a")
			set(OTHER_LIBRARIES pthread dl)
			set(BASS_LIBRARY "/home/trent/code/crystal-picnic/build/libbass.so")
			set(BASSMIDI_LIBRARY "/home/trent/code/crystal-picnic/build/libbassmidi.so")
		endif()

		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,\$ORIGIN/stuff")

		set(MAIN_LIBS
			crystalpicnic-engine
			tgui2-static
			my_load_bitmap
			${ALLEGRO_LIBRARIES}
			Xcursor Xi Xinerama Xrandr GLU GL SM ICE X11 Xext Xpm
			${LUA_LIBRARY}
			${PNG_LIBRARY}
			${FREETYPE_LIBRARY}
			${ZLIB_LIBRARY}
			${BASS_LIBRARY}
			${BASSMIDI_LIBRARY}
			${POLY2TRI_LIBRARY}
			${BROTLI_LIBRARY}
			${OTHER_LIBRARIES}
			${STEAMWORKS_LIBRARY}
			pthread
		)
	endif()
endif()

target_link_libraries(CrystalPicnic ${MAIN_LIBS})

# Tools

if(BATTLED2)
   add_subdirectory(tools/apps/battled2)
endif(BATTLED2)

if(TOOLS)
   add_subdirectory(tools)
endif(TOOLS)
