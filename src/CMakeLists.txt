cmake_minimum_required(VERSION 3.10)

project(cdplayer)

add_executable(cdplayer cdplayer.c)

target_link_libraries(cdplayer vlc ncurses)
