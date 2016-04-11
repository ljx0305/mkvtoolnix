/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_EXTRACT_XTR_WEBVTT_H
#define MTX_EXTRACT_XTR_WEBVTT_H

#include "common/common_pch.h"

#include "common/xml/xml.h"
#include "extract/xtr_base.h"

class xtr_webvtt_c: public xtr_base_c {
protected:
  unsigned int m_num_entries{};

public:
  xtr_webvtt_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);

  virtual const char *get_container_name() {
    return Y("WebVTT subtitles");
  };
};

#endif  // MTX_EXTRACT_XTR_WEBVTT_H
