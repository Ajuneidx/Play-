#include "ChdImageStream.h"
#include <libchdr/chd.h>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include "ChdStreamSupport.h"

CChdImageStream::CChdImageStream(Framework::CStream* baseStream)
    : m_baseStream(baseStream)
{
	m_file = ChdStreamSupport::CreateFileFromStream(baseStream);
	chd_error result = chd_open_file(m_file, CHD_OPEN_READ, nullptr, &m_chd);
	if(result != CHDERR_NONE)
	{
		core_ffree(m_file);
		throw std::runtime_error("Failed to open CHD file.");
	}
	auto header = chd_get_header(m_chd);
	m_hunkCount = header->hunkcount;
	m_hunkSize = header->hunkbytes;
	m_hunkBuffer.resize(m_hunkSize);
}

CChdImageStream::~CChdImageStream()
{
	chd_close(m_chd);
	core_ffree(m_file);
}

void CChdImageStream::Seek(int64 position, Framework::STREAM_SEEK_DIRECTION origin)
{
	switch(origin)
	{
	case Framework::STREAM_SEEK_CUR:
		m_position += position;
		break;
	case Framework::STREAM_SEEK_SET:
		m_position = position;
		break;
	case Framework::STREAM_SEEK_END:
		m_position = GetTotalSize() + position;
		break;
	}
}

uint64 CChdImageStream::Tell()
{
	return m_position;
}

bool CChdImageStream::IsEOF()
{
	return m_position >= GetTotalSize();
}

uint64 CChdImageStream::Read(void* buffer, uint64 size)
{
	uint32 hunkPosition = m_position % m_hunkSize;
	uint32 hunkRemainSize = m_hunkSize - hunkPosition;
	assert((hunkPosition + size) <= m_hunkSize);
	uint32 hunkIdx = m_position / m_hunkSize;
	if(hunkIdx != m_hunkBufferIdx)
	{
		chd_read(m_chd, hunkIdx, m_hunkBuffer.data());
		m_hunkBufferIdx = hunkIdx;
	}
	memcpy(buffer, m_hunkBuffer.data() + hunkPosition, size);
	m_position += size;
	return size;
}

uint64 CChdImageStream::Write(const void* buffer, uint64 size)
{
	throw std::runtime_error("Not supported.");
}

uint64 CChdImageStream::GetTotalSize() const
{
	return m_hunkCount * m_hunkSize;
}
