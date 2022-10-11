/*
 * Copyright (c) 2021, Hunter Salyer <thefalsehonesty@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MatroskaReader.h"
#include <AK/Error.h>
#include <AK/Function.h>
#include <AK/Utf8View.h>
#include <LibCore/MappedFile.h>

namespace Video {

constexpr u32 EBML_MASTER_ELEMENT_ID = 0x1A45DFA3;
constexpr u32 SEGMENT_ELEMENT_ID = 0x18538067;
constexpr u32 DOCTYPE_ELEMENT_ID = 0x4282;
constexpr u32 DOCTYPE_VERSION_ELEMENT_ID = 0x4287;
constexpr u32 SEGMENT_INFORMATION_ELEMENT_ID = 0x1549A966;
constexpr u32 TRACK_ELEMENT_ID = 0x1654AE6B;
constexpr u32 CLUSTER_ELEMENT_ID = 0x1F43B675;
constexpr u32 TIMESTAMP_SCALE_ID = 0x2AD7B1;
constexpr u32 MUXING_APP_ID = 0x4D80;
constexpr u32 WRITING_APP_ID = 0x5741;
constexpr u32 TRACK_ENTRY_ID = 0xAE;
constexpr u32 TRACK_NUMBER_ID = 0xD7;
constexpr u32 TRACK_UID_ID = 0x73C5;
constexpr u32 TRACK_TYPE_ID = 0x83;
constexpr u32 TRACK_LANGUAGE_ID = 0x22B59C;
constexpr u32 TRACK_CODEC_ID = 0x86;
constexpr u32 TRACK_VIDEO_ID = 0xE0;
constexpr u32 TRACK_AUDIO_ID = 0xE1;
constexpr u32 PIXEL_WIDTH_ID = 0xB0;
constexpr u32 PIXEL_HEIGHT_ID = 0xBA;
constexpr u32 CHANNELS_ID = 0x9F;
constexpr u32 BIT_DEPTH_ID = 0x6264;
constexpr u32 SIMPLE_BLOCK_ID = 0xA3;
constexpr u32 TIMESTAMP_ID = 0xE7;

ErrorOr<NonnullOwnPtr<MatroskaDocument>> MatroskaReader::parse_matroska_from_file(StringView path)
{
    auto mapped_file = TRY(Core::MappedFile::map(path));
    return TRY(parse_matroska_from_data((u8*)mapped_file->data(), mapped_file->size()));
}

ErrorOr<NonnullOwnPtr<MatroskaDocument>> MatroskaReader::parse_matroska_from_data(u8 const* data, size_t size)
{
    MatroskaReader reader(data, size);
    return TRY(reader.parse());
}

ErrorOr<NonnullOwnPtr<MatroskaDocument>> MatroskaReader::parse()
{
    auto first_element_id = TRY(m_streamer.read_variable_size_integer(false));
    dbgln_if(MATROSKA_TRACE_DEBUG, "First element ID is {:#010x}\n", first_element_id);
    if (first_element_id != EBML_MASTER_ELEMENT_ID)
        return Error::from_string_literal("First element is not EBML master element");

    auto header = TRY(parse_ebml_header());

    dbgln_if(MATROSKA_DEBUG, "Parsed EBML header");

    auto root_element_id = TRY(m_streamer.read_variable_size_integer(false));
    if (root_element_id != SEGMENT_ELEMENT_ID)
        return Error::from_string_literal("Root element is not segment element");

    auto matroska_document = make<MatroskaDocument>(header);

    TRY(parse_segment_elements(*matroska_document));

    return matroska_document;
}

ErrorOr<void> MatroskaReader::parse_master_element([[maybe_unused]] StringView element_name, Function<ErrorOr<void>(u64)> element_consumer)
{
    auto element_data_size = TRY(m_streamer.read_variable_size_integer());
    
    dbgln_if(MATROSKA_DEBUG, "{} has {} octets of data.", element_name, element_data_size);

    m_streamer.push_octets_read();
    while (m_streamer.octets_read() < element_data_size) {
        dbgln_if(MATROSKA_TRACE_DEBUG, "====== Reading  element ======");

        auto element_id = TRY(m_streamer.read_variable_size_integer(false));
        dbgln_if(MATROSKA_TRACE_DEBUG, "{:s} element ID is {:#010x}\n", element_name, element_id);

        TRY(element_consumer(element_id));

        dbgln_if(MATROSKA_TRACE_DEBUG, "Read {} octets of the {} so far.", m_streamer.octets_read(), element_name);
    }
    m_streamer.pop_octets_read();

    return {};
}

ErrorOr<EBMLHeader> MatroskaReader::parse_ebml_header()
{
    EBMLHeader header;
    TRY(parse_master_element("Header"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == DOCTYPE_ELEMENT_ID) {
            header.doc_type = TRY(read_string_element());
            dbgln_if(MATROSKA_DEBUG, "Read DocType attribute: {}", header.doc_type);
        } else if (element_id == DOCTYPE_VERSION_ELEMENT_ID) {
            header.doc_type_version = TRY(read_u64_element());
            dbgln_if(MATROSKA_DEBUG, "Read DocTypeVersion attribute: {}", header.doc_type_version);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    return header;
}

ErrorOr<void> MatroskaReader::parse_segment_elements(MatroskaDocument& matroska_document)
{
    dbgln_if(MATROSKA_DEBUG, "Parsing segment elements");
    TRY(parse_master_element("Segment"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == SEGMENT_INFORMATION_ELEMENT_ID) {
            auto segment_information = TRY(parse_information());
            matroska_document.set_segment_information(move(segment_information));
        } else if (element_id == TRACK_ELEMENT_ID) {
            TRY(parse_tracks(matroska_document));
        } else if (element_id == CLUSTER_ELEMENT_ID) {
            auto cluster = TRY(parse_cluster());
            matroska_document.clusters().append(move(cluster));
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    dbgln_if(MATROSKA_DEBUG, "Parsed segment elements");

    return {};
}

ErrorOr<NonnullOwnPtr<SegmentInformation>> MatroskaReader::parse_information()
{
    auto segment_information = make<SegmentInformation>();
    TRY(parse_master_element("Segment Information"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == TIMESTAMP_SCALE_ID) {
            auto timestamp_scale = TRY(read_u64_element());
            segment_information->set_timestamp_scale(timestamp_scale);
            dbgln_if(MATROSKA_DEBUG, "Read TimestampScale attribute: {}", timestamp_scale);
        } else if (element_id == MUXING_APP_ID) {
            auto muxing_app = TRY(read_string_element());
            segment_information->set_muxing_app(muxing_app);
            dbgln_if(MATROSKA_DEBUG, "Read MuxingApp attribute: {}", muxing_app);
        } else if (element_id == WRITING_APP_ID) {
            auto writing_app = TRY(read_string_element());
            segment_information->set_writing_app(writing_app);
            dbgln_if(MATROSKA_DEBUG, "Read WritingApp attribute: {}", writing_app);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    return segment_information;
}

ErrorOr<void> MatroskaReader::parse_tracks(MatroskaDocument& matroska_document)
{
    return TRY(parse_master_element("Tracks"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == TRACK_ENTRY_ID) {
            dbgln_if(MATROSKA_DEBUG, "Parsing track");
            auto track_entry = TRY(parse_track_entry());
            auto track_number = track_entry->track_number();
            matroska_document.add_track(track_number, move(track_entry));
            dbgln_if(MATROSKA_DEBUG, "Track {} added to document", track_number);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));
}

ErrorOr<NonnullOwnPtr<TrackEntry>> MatroskaReader::parse_track_entry()
{
    auto track_entry = make<TrackEntry>();
    TRY(parse_master_element("Track"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == TRACK_NUMBER_ID) {
            auto track_number = TRY(read_u64_element());
            track_entry->set_track_number(track_number);
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read TrackNumber attribute: {}", track_number);
        } else if (element_id == TRACK_UID_ID) {
            auto track_uid = TRY(read_u64_element());
            track_entry->set_track_uid(track_uid);
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read TrackUID attribute: {}", track_uid);
        } else if (element_id == TRACK_TYPE_ID) {
            auto track_type = TRY(read_u64_element());
            track_entry->set_track_type(static_cast<TrackEntry::TrackType>(track_type));
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read TrackType attribute: {}", track_type);
        } else if (element_id == TRACK_LANGUAGE_ID) {
            auto language = TRY(read_string_element());
            track_entry->set_language(language);
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read Track's Language attribute: {}", language);
        } else if (element_id == TRACK_CODEC_ID) {
            auto codec_id = TRY(read_string_element());
            track_entry->set_codec_id(codec_id);
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read Track's CodecID attribute: {}", codec_id);
        } else if (element_id == TRACK_VIDEO_ID) {
            auto video_track = TRY(parse_video_track_information());
            track_entry->set_video_track(video_track);
        } else if (element_id == TRACK_AUDIO_ID) {
            auto audio_track = TRY(parse_audio_track_information());
            track_entry->set_audio_track(audio_track);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    return track_entry;
}

ErrorOr<TrackEntry::VideoTrack> MatroskaReader::parse_video_track_information()
{
    TrackEntry::VideoTrack video_track {};

    TRY(parse_master_element("VideoTrack"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == PIXEL_WIDTH_ID) {
            auto pixel_width = TRY(read_u64_element());
            video_track.pixel_width = pixel_width;
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read VideoTrack's PixelWidth attribute: {}", pixel_width);
        } else if (element_id == PIXEL_HEIGHT_ID) {
            auto pixel_height = TRY(read_u64_element());
            video_track.pixel_height = pixel_height;
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read VideoTrack's PixelHeight attribute: {}", pixel_height);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    return video_track;
}

ErrorOr<TrackEntry::AudioTrack> MatroskaReader::parse_audio_track_information()
{
    TrackEntry::AudioTrack audio_track {};

    TRY(parse_master_element("AudioTrack"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == CHANNELS_ID) {
            audio_track.channels = TRY(read_u64_element());
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read AudioTrack's Channels attribute: {}", audio_track.channels);
        } else if (element_id == BIT_DEPTH_ID) {
            audio_track.bit_depth = TRY(read_u64_element());
            dbgln_if(MATROSKA_TRACE_DEBUG, "Read AudioTrack's BitDepth attribute: {}", audio_track.bit_depth);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    return audio_track;
}

ErrorOr<NonnullOwnPtr<Cluster>> MatroskaReader::parse_cluster()
{
    auto cluster = make<Cluster>();

    TRY(parse_master_element("Cluster"sv, [&](u64 element_id) -> ErrorOr<void> {
        if (element_id == SIMPLE_BLOCK_ID) {
            auto simple_block = TRY(parse_simple_block());
            cluster->blocks().append(move(simple_block));
        } else if (element_id == TIMESTAMP_ID) {
            auto timestamp = TRY(read_u64_element());
            cluster->set_timestamp(timestamp);
        } else {
            TRY(read_unknown_element());
        }

        return {};
    }));

    return cluster;
}

ErrorOr<NonnullOwnPtr<Block>> MatroskaReader::parse_simple_block()
{
    auto block = make<Block>();

    auto content_size = TRY(m_streamer.read_variable_size_integer());

    auto octets_read_before_track_number = m_streamer.octets_read();
    auto track_number = TRY(m_streamer.read_variable_size_integer());
    block->set_track_number(track_number);

    if (m_streamer.remaining() < 3)
        return Error::from_string_literal("Not enough data to read SimpleBlock");
    block->set_timestamp(m_streamer.read_i16());

    auto flags = m_streamer.read_octet();
    block->set_only_keyframes(flags & (1u << 7u));
    block->set_invisible(flags & (1u << 3u));
    block->set_lacing(static_cast<Block::Lacing>((flags & 0b110u) >> 1u));
    block->set_discardable(flags & 1u);

    auto total_frame_content_size = content_size - (m_streamer.octets_read() - octets_read_before_track_number);
    if (block->lacing() == Block::Lacing::EBML) {
        auto octets_read_before_frame_sizes = m_streamer.octets_read();
        auto frame_count = m_streamer.read_octet() + 1;
        Vector<u64> frame_sizes;
        frame_sizes.ensure_capacity(frame_count);

        u64 frame_size_sum = 0;
        u64 previous_frame_size;
        auto first_frame_size = TRY(m_streamer.read_variable_size_integer());
        frame_sizes.append(first_frame_size);
        frame_size_sum += first_frame_size;
        previous_frame_size = first_frame_size;

        for (int i = 0; i < frame_count - 2; i++) {
            auto frame_size_difference = TRY(m_streamer.read_variable_sized_signed_integer());
            u64 frame_size;
            if (frame_size_difference < 0)
                frame_size = previous_frame_size - (-frame_size_difference);
            else
                frame_size = previous_frame_size + frame_size_difference;
            frame_sizes.append(frame_size);
            frame_size_sum += frame_size;
            previous_frame_size = frame_size;
        }
        frame_sizes.append(total_frame_content_size - frame_size_sum - (m_streamer.octets_read() - octets_read_before_frame_sizes));

        for (int i = 0; i < frame_count; i++) {
            auto current_frame_size = frame_sizes.at(i);
            auto frame_result = TRY(ByteBuffer::copy(m_streamer.data(), current_frame_size));
            block->add_frame(move(frame_result));
            m_streamer.drop_octets(current_frame_size);
        }
    } else if (block->lacing() == Block::Lacing::FixedSize) {
        auto frame_count = m_streamer.read_octet() + 1;
        auto individual_frame_size = total_frame_content_size / frame_count;
        for (int i = 0; i < frame_count; i++) {
            auto frame_result = TRY(ByteBuffer::copy(m_streamer.data(), individual_frame_size));
            block->add_frame(move(frame_result));
            m_streamer.drop_octets(individual_frame_size);
        }
    } else {
        auto frame_result = TRY(ByteBuffer::copy(m_streamer.data(), total_frame_content_size));
        block->add_frame(move(frame_result));
        m_streamer.drop_octets(total_frame_content_size);
    }
    return block;
}

ErrorOr<String> MatroskaReader::read_string_element()
{
    auto string_length = TRY(m_streamer.read_variable_size_integer());
    if (m_streamer.remaining() < string_length)
        return Error::from_string_literal("Not enough data to read string element");
    auto string_value = String(m_streamer.data_as_chars(), string_length);
    m_streamer.drop_octets(string_length);
    return string_value;
}

ErrorOr<u64> MatroskaReader::read_u64_element()
{
    auto integer_length = TRY(m_streamer.read_variable_size_integer());
    if (m_streamer.remaining() < integer_length)
        return Error::from_string_literal("Not enough data to read u64 element");
    u64 result = 0;
    for (size_t i = 0; i < integer_length; i++) {
        if (!m_streamer.has_octet())
            return Error::from_string_literal("Not enough data to read u64 element");
        result = (result << 8u) + m_streamer.read_octet();
    }
    return result;
}

ErrorOr<void> MatroskaReader::read_unknown_element()
{
    auto element_length = TRY(m_streamer.read_variable_size_integer());
    if (m_streamer.remaining() < element_length)
        return Error::from_string_literal("Not enough data to read element");

    m_streamer.drop_octets(element_length);
    return {};
}

}
