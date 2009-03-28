/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions and helper functions for TRUEHD data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef __TRUEHD_COMMON_H
#define __TRUEHD_COMMON_H

#include "os.h"

#include <deque>

#include "byte_buffer.h"
#include "common_memory.h"
#include "smart_pointers.h"

#define TRUEHD_SYNC_WORD 0xf8726fba

struct truehd_frame_t {
  enum {
    invalid,
    normal,
    sync,
    ac3,
  } m_type;

  int m_size;
  int m_sampling_rate;
  int m_channels;
  int m_samples_per_frame;

  memory_cptr m_data;

  truehd_frame_t()
    : m_type(invalid)
    , m_size(0)
    , m_sampling_rate(0)
    , m_channels(0)
    , m_samples_per_frame(0)
  { };
};
typedef counted_ptr<truehd_frame_t> truehd_frame_cptr;

class truehd_parser_c {
protected:
  enum {
    state_unsynced,
    state_synced,
  } m_sync_state;

  byte_buffer_c m_buffer;
  std::deque<truehd_frame_cptr> m_frames;

public:
  truehd_parser_c();

  virtual void add_data(const unsigned char *new_data, unsigned int new_size);
  virtual bool frame_available();
  virtual truehd_frame_cptr get_next_frame();

protected:
  virtual unsigned int resync(unsigned int offset);
};
typedef counted_ptr<truehd_parser_c> truehd_parser_cptr;

#endif // __TRUEHD_COMMON_H
