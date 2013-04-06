// Comment.cpp

#include "StdAfx.h"
#include "Includes.h"

CComment::CComment()
{
}

CComment::~CComment()
{
}

void CComment::Delete()
{
	if (m_comment != NULL)
	{
		free(m_comment);
		m_comment = NULL;
	}
	delete this;
}

CComment* CComment::Create(LPCTSTR comment)
{
	CComment* retval = new CComment();
	retval->Init(comment);
	return retval;
}

void CComment::SetComment(LPCTSTR comment)
{
	if (m_comment != NULL)
	{
		free(m_comment);
	}
	if (comment == NULL)
	{
		m_comment = NULL;
	}
	else
	{
		m_comment = (char*)malloc(strlen(comment) + 1);
		strcpy(m_comment, comment);
	}
}

LPCTSTR CComment::GetComment()
{
	return m_comment;
}

void CComment::SetNext(CComment* next)
{
	m_next = next;
}

CComment* CComment::GetNext()
{
	return m_next;
}

void CComment::Init(LPCTSTR comment)
{
	m_next = NULL;
	m_comment = NULL;
	SetComment(comment);
}

void CComment::Write(CTxtFile* file)
{
	if (m_comment != NULL)
	{
		file->WriteComment(m_comment);
	}
}
