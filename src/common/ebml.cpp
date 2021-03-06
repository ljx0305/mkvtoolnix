/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions that need libebml/libmatroska

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxChapters.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/memory.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
#include "common/tags/tags.h"

using namespace libebml;

EbmlElement *
empty_ebml_master(EbmlElement *e) {
  EbmlMaster *m;

  m = dynamic_cast<EbmlMaster *>(e);
  if (!m)
    return e;

  while (m->ListSize() > 0) {
    delete (*m)[0];
    m->Remove(0);
  }

  return m;
}

EbmlElement *
create_ebml_element(const EbmlCallbacks &callbacks,
                    const EbmlId &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(callbacks);
  size_t i;

//   if (id == EbmlId(*parent))
//     return empty_ebml_master(&parent->Generic().Create());

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return empty_ebml_master(&EBML_SEM_CREATE(EBML_CTX_IDX(context,i)));

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    EbmlElement *e;

    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;

    e = create_ebml_element(EBML_CTX_IDX_INFO(context, i), id);
    if (e)
      return e;
  }

  return nullptr;
}

const EbmlCallbacks *
find_ebml_callbacks(const EbmlCallbacks &base,
                    const EbmlId &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlCallbacks *result;
  size_t i;

  if (EBML_INFO_ID(base) == id)
    return &base;

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return &EBML_CTX_IDX_INFO(context, i);

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = find_ebml_callbacks(EBML_CTX_IDX_INFO(context, i), id);
    if (result)
      return result;
  }

  return nullptr;
}

const EbmlCallbacks *
find_ebml_callbacks(const EbmlCallbacks &base,
                    const char *debug_name) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlCallbacks *result;
  size_t i;

  if (!strcmp(debug_name, EBML_INFO_NAME(base)))
    return &base;

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (!strcmp(debug_name, EBML_INFO_NAME(EBML_CTX_IDX_INFO(context, i))))
      return &EBML_CTX_IDX_INFO(context, i);

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = find_ebml_callbacks(EBML_CTX_IDX_INFO(context, i), debug_name);
    if (result)
      return result;
  }

  return nullptr;
}

const EbmlCallbacks *
find_ebml_parent_callbacks(const EbmlCallbacks &base,
                           const EbmlId &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlCallbacks *result;
  size_t i;

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return &base;

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = find_ebml_parent_callbacks(EBML_CTX_IDX_INFO(context, i), id);
    if (result)
      return result;
  }

  return nullptr;
}

const EbmlSemantic *
find_ebml_semantic(const EbmlCallbacks &base,
                   const EbmlId &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlSemantic *result;
  size_t i;

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return &EBML_CTX_IDX(context,i);

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = find_ebml_semantic(EBML_CTX_IDX_INFO(context, i), id);
    if (result)
      return result;
  }

  return nullptr;
}

EbmlMaster *
sort_ebml_master(EbmlMaster *m) {
  if (!m)
    return m;

  int first_element = -1;
  int first_master  = -1;
  size_t i;
  for (i = 0; i < m->ListSize(); i++) {
    if (dynamic_cast<EbmlMaster *>((*m)[i]) && (-1 == first_master))
      first_master = i;
    else if (!dynamic_cast<EbmlMaster *>((*m)[i]) && (-1 != first_master) && (-1 == first_element))
      first_element = i;
    if ((first_master != -1) && (first_element != -1))
      break;
  }

  if (first_master == -1)
    return m;

  while (first_element != -1) {
    EbmlElement *e = (*m)[first_element];
    m->Remove(first_element);
    m->InsertElement(*e, first_master);
    first_master++;
    for (first_element++; first_element < static_cast<int>(m->ListSize()); first_element++)
      if (!dynamic_cast<EbmlMaster *>((*m)[first_element]))
        break;
    if (first_element >= static_cast<int>(m->ListSize()))
      first_element = -1;
  }

  for (i = 0; i < m->ListSize(); i++)
    if (dynamic_cast<EbmlMaster *>((*m)[i]))
      sort_ebml_master(dynamic_cast<EbmlMaster *>((*m)[i]));

  return m;
}

void
move_children(EbmlMaster &source,
              EbmlMaster &destination) {
  for (auto child : source)
    destination.PushElement(*child);
}

// ------------------------------------------------------------------------

int64_t
kt_get_default_duration(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackDefaultDuration>(track);
}

int64_t
kt_get_number(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackNumber>(track);
}

int64_t
kt_get_uid(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackUID>(track);
}

std::string
kt_get_codec_id(KaxTrackEntry &track) {
  return FindChildValue<KaxCodecID>(track);
}

std::string
kt_get_language(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackLanguage>(track);
}

int
kt_get_max_blockadd_id(KaxTrackEntry &track) {
  return FindChildValue<KaxMaxBlockAdditionID>(track);
}

int
kt_get_a_channels(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioChannels>(audio, 1u) : 1;
}

double
kt_get_a_sfreq(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioSamplingFreq>(audio, 8000.0) : 8000.0;
}

double
kt_get_a_osfreq(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioOutputSamplingFreq>(audio, 8000.0) : 8000.0;
}

int
kt_get_a_bps(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioBitDepth, int>(audio, -1) : -1;
}

int
kt_get_v_pixel_width(KaxTrackEntry &track) {
  auto video = FindChild<KaxTrackVideo>(track);
  return video ? FindChildValue<KaxVideoPixelWidth>(video) : 0;
}

int
kt_get_v_pixel_height(KaxTrackEntry &track) {
  auto video = FindChild<KaxTrackVideo>(track);
  return video ? FindChildValue<KaxVideoPixelHeight>(video) : 0;
}

EbmlElement *
find_ebml_element_by_id(EbmlMaster *master,
                        const EbmlId &id) {
  for (auto child : *master)
    if (EbmlId(*child) == id)
      return child;

  return nullptr;
}

std::pair<EbmlMaster *, size_t>
find_element_in_master(EbmlMaster *master,
                       EbmlElement *element_to_find) {
  if (!master || !element_to_find)
    return std::make_pair<EbmlMaster *, size_t>(nullptr, 0);

  auto &elements = master->GetElementList();
  auto itr       = brng::find(elements, element_to_find);

  if (itr != elements.end())
    return std::make_pair(master, std::distance(elements.begin(), itr));

  for (auto &sub_element : elements) {
    auto sub_master = dynamic_cast<EbmlMaster *>(sub_element);
    if (!sub_master)
      continue;

    auto result = find_element_in_master(sub_master, element_to_find);
    if (result.first)
      return result;
  }

  return std::make_pair<EbmlMaster *, size_t>(nullptr, 0);
}

void
fix_mandatory_elements(EbmlElement *master) {
  if (dynamic_cast<KaxInfo *>(master))
    fix_mandatory_segmentinfo_elements(master);

  else if (dynamic_cast<KaxTracks *>(master))
    fix_mandatory_segment_tracks_elements(master);

  else if (dynamic_cast<KaxTags *>(master))
    mtx::tags::fix_mandatory_elements(master);

  else if (dynamic_cast<KaxChapters *>(master))
    fix_mandatory_chapter_elements(master);
}

void
remove_voids_from_master(EbmlElement *element) {
  auto master = dynamic_cast<EbmlMaster *>(element);
  if (master)
    DeleteChildren<EbmlVoid>(master);
}

int
write_ebml_element_head(mm_io_c &out,
                        EbmlId const &id,
                        int64_t content_size) {
	int id_size    = EBML_ID_LENGTH(id);
	int coded_size = CodedSizeLength(content_size, 0);
  uint8_t buffer[4 + 8];

	id.Fill(buffer);
	CodedValueLength(content_size, coded_size, &buffer[id_size]);

  return out.write(buffer, id_size + coded_size);
}
