#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <ncurses.h>
#include <vlc/vlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_list.h>
#include <vlc/libvlc_media_list_player.h>
#include <vlc/libvlc_media_player.h>

#define BTN_NONE 0
#define BTN_PP 1
#define BTN_PREV 2
#define BTN_NEXT 3
#define BTN_EJECT 4

char *dev_path = "/dev/sr0"; // CD drive device path
int dev_stat; // CD drive device status
int prev_dev_stat; // Last CD drive device status

libvlc_instance_t *vlc_inst;
libvlc_media_player_t *mediaplayer;
libvlc_media_t *media;
libvlc_media_list_t *medialist;
libvlc_media_list_player_t *medialistplayer;

int kbhit() {
  int ch = getch();

  if (ch != ERR) {
    ungetch(ch);
    return 1;
  } else {
    return 0;
  }
}

int get_cd_stat(char *path) {
  int fd = open(path, O_RDONLY | O_NONBLOCK);
  int result = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_NONE);
  close(fd);
  return result;
}

int send_cd_cmd(char *path, int cmd, int in) {
  int fd = open(path, O_RDONLY | O_NONBLOCK);
  int result = ioctl(fd, cmd, in);
  close(fd);
  return result;
}

char *conv_cd_stat_to_text(int *stat) {
  switch (*stat) {
    case CDS_NO_INFO:
      return "No info.";
    case CDS_NO_DISC:
      return "No disc.";
    case CDS_TRAY_OPEN:
      return "Tray open.";
    case CDS_DRIVE_NOT_READY:
      return "Drive not ready.";
    case CDS_DISC_OK:
      return "Disk OK.";
    default:
      return "Unknown status.";
  }
}

void setup_medialistplayer() {
  /* Create a media player environment */
  mediaplayer = libvlc_media_player_new(vlc_inst);
  medialist = libvlc_media_list_new(vlc_inst);
  medialistplayer = libvlc_media_list_player_new(vlc_inst);
  libvlc_media_list_player_set_media_player(medialistplayer, mediaplayer);
}

void load_media(char *loc) {
  /* Read media from loc and load into a media list for
   * the medialistplayer */
  media = libvlc_media_new_location(vlc_inst, loc);
  libvlc_media_list_add_media(medialist, media);
  libvlc_media_list_player_set_media_list(medialistplayer, medialist);

  libvlc_media_release(media);
  media = NULL;
}

void cleanup_medialistplayer() {
  /* Cleanup all the mem allocated and processes started
   * in setup_medialistplayer */
  libvlc_media_list_player_release(medialistplayer);
  medialistplayer = NULL;
  libvlc_media_list_release(medialist);
  medialist = NULL;
  libvlc_media_player_release(mediaplayer);
  mediaplayer = NULL;
}

int get_btn() {
  int cmd = BTN_NONE;

  if (kbhit()) {
    int ch = getch();
    if (ch == 0x20) // Space
      cmd = BTN_PP;
    if (ch == 0x1b) // Esc
      cmd = BTN_EJECT;
    if (ch == '\033') {
      getch();
      switch (getch()) {
        case 'C': // Right arrow
          cmd = BTN_NEXT;
          break;
        case 'D': // Left arrow
          cmd = BTN_PREV;
          break;
      }
    }
  }

  refresh();
  return cmd;
}

void handle_buttons() {
    switch (get_btn()) {
      case BTN_EJECT:
        libvlc_media_list_player_stop(medialistplayer);
        send_cd_cmd(dev_path, CDROM_LOCKDOOR, 0);
        send_cd_cmd(dev_path, CDROMEJECT, 0);
        break;
      case BTN_NEXT:
        libvlc_media_list_player_next(medialistplayer);
        break;
      case BTN_PREV:
        libvlc_media_list_player_previous(medialistplayer);
        break;
      case BTN_PP:
        libvlc_media_list_player_pause(medialistplayer);
        break;
    }
}

void print_status_line(char *first, char *second, char *third) {
  printf("\r%-20s%-20s%-20s", first, second, third);
  fflush(stdout);
}

void init_ncurses() {
  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
}

void control_loop() {
  libvlc_state_t player_state;

  printf("\r");
  prev_dev_stat = dev_stat;
  dev_stat = get_cd_stat(dev_path);
  printf("%-20s", conv_cd_stat_to_text(&dev_stat));

  /* Skip the loop if the disc isn't being read
   * and pause the player if it exists */
  if (dev_stat != CDS_DISC_OK) {
    if (medialistplayer != NULL)
      libvlc_media_list_player_set_pause(medialistplayer, 1);
    return;
  }

  /* Load the inserted CD into medialistplayer and play */
  if (dev_stat == CDS_DISC_OK && prev_dev_stat != CDS_DISC_OK) {
    load_media("cdda://");
    libvlc_media_list_player_play_item_at_index(medialistplayer, 0);
  }

  player_state = libvlc_media_list_player_get_state(medialistplayer);

  /* Stop playing and eject the disc upon error or player completion */
  if (dev_stat != CDS_TRAY_OPEN
      && (player_state == libvlc_Error
        || player_state == libvlc_Ended)) {
    libvlc_media_list_player_stop(medialistplayer);
    send_cd_cmd(dev_path, CDROM_LOCKDOOR, 0);
    send_cd_cmd(dev_path, CDROMEJECT, 0);
  }

  handle_buttons();
  printf("%-20d", player_state);
}

int main() {
  init_ncurses();

  /* Load the VLC engine */
  vlc_inst = libvlc_new(0, NULL);
  setup_medialistplayer();

  while (1) {
    control_loop();
  }

  cleanup_medialistplayer();
  libvlc_release(vlc_inst);
}
