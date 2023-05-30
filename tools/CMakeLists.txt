project(projects)

if(APPLE)
   set(EXTRA_ALLEGRO_LIBS allegro_main${ALLEG_SUFFIX})
else(APPLE)
   set(EXTRA_ALLEGRO_LIBS )
endif(APPLE)

if(APPLE)
	FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
	FIND_LIBRARY(FOUNDATION_LIBRARY Foundation)
	FIND_LIBRARY(APPSERVICES_LIBRARY ApplicationServices)
	FIND_LIBRARY(APPKIT_LIBRARY AppKit)
	FIND_LIBRARY(IOKIT_LIBRARY IOKit)
	FIND_LIBRARY(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	MARK_AS_ADVANCED(OPENGL_LIBRARY)
	MARK_AS_ADVANCED(FOUNDATION_LIBRARY)
	MARK_AS_ADVANCED(APPSERVICES_LIBRARY)
	MARK_AS_ADVANCED(IOKIT_LIBRARY)
	SET(ALLEGRO_LIBS ${IOKIT_LIBRARY} ${OPENGL_LIBRARY} ${FOUNDATION_LIBRARY} ${APPSERVICES_LIBRARY} /Users/trent/code/local/lib/libfreetype.a ${APPKIT_LIBRARY} ${AUDIOTOOLBOX_LIBRARY} allegro${ALLEG_SUFFIX} allegro_memfile${ALLEG_SUFFIX} allegro_primitives${ALLEG_SUFFIX} allegro_image${ALLEG_SUFFIX} ${EXTRA_ALLEGRO_LIBS} /Users/trent/code/local/lib/libz.a /Users/trent/code/local/lib/libpng.a)
else(APPLE)
	if(WIN32)
		set(EXTRA_LIBS "opengl32" "dxguid" "dinput8" "xinput" "dsound" "d3d9" "kernel32" "user32" "gdi32" "comdlg32" "ole32" "winmm" "psapi" "gdiplus" "uuid" "shlwapi")
		if(GNU)
			set(ZLIB_LIB_NAME "z")
		else()
			set(ZLIB_LIB_NAME "zlibstatic")
		endif()
		set(ALLEGRO_LIBS allegro_monolith-static freetype ${ZLIB_LIB_NAME} ${EXTRA_LIBS})
	else(WIN32)
		if(RASPBERRYPI)
			set(ALLEGRO_LIBS
				allegro_monolith-static
				${ALLEGRO_MAIN_LIB}
				m pthread SM ICE X11 Xext Xcursor Xinerama Xrandr bcm_host GLESv2 EGL
				/usr/lib/arm-linux-gnueabihf/libpng.a
				/usr/lib/arm-linux-gnueabihf/libfreetype.a
				/usr/lib/arm-linux-gnueabihf/libz.a
			)
		else(RASPBERRYPI)
			set(ALLEGRO_LIBS
				allegro_monolith-static
				m pthread SM ICE X11 Xext Xcursor Xinerama Xrandr Xi GL GLU Xpm
				png
				freetype
				z
			)
		endif()
	endif(WIN32)
endif(APPLE)

function(add_tool target libs)
   add_executable(${target} ${ARGN})
   target_link_libraries(${target} ${libs})
   if(WIN32 AND NOT SHARED_LIBS)
   	set_target_properties(${target} PROPERTIES COMPILE_FLAGS "/DALLEGRO_STATICLINK")
   endif()
endfunction(add_tool)

add_tool(packtiles2 "${ALLEGRO_LIBS}" utils/packtiles2.cpp)

if(NOT RASPBERRYPI)
	if(NOT WIN32 AND NOT APPLE)
		#add_tool(lineed "${ALLEGRO_LIBS};allegro_dialog${ALLEG_SUFFIX};gtkglext-x11-1.0;gtk-x11-2.0;gdk-x11-2.0;atk-1.0;gio-2.0;pangoft2-1.0;pangocairo-1.0;gdk_pixbuf-2.0;cairo;pango-1.0;freetype;fontconfig;gobject-2.0;glib-2.0" apps/lineed.cpp)
	else(NOT WIN32 AND NOT APPLE)
		add_tool(lineed "${ALLEGRO_LIBS};allegro_dialog${ALLEG_SUFFIX}" apps/lineed.cpp)
	endif(NOT WIN32 AND NOT APPLE)
endif()

add_tool(boneed "${ALLEGRO_LIBS}" apps/boneed.cpp)

add_tool(level_optimizer "${ALLEGRO_LIBS}" utils/level_optimizer.cpp)
