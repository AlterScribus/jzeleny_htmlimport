/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
 *   Copyright (C) 2004 by Riku Leino                                      *
 *   tsoots@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef HTMLREADER_H
#define HTMLREADER_H

#include "scconfig.h"

#include <vector>
#include <libxml/HTMLparser.h>

#include <QString>
#include <QXmlAttributes>

#include <gtparagraphstyle.h>
#include <gtwriter.h>

/*! \brief Parse and import a HTML file.
Supported tags: P, CENTER, BR, A, UL, OL, LI, H1, H2, H3, H4,
B, STRONG, I, EM, CODE, BODY, PRE, IMG, SUB, SUP, DEL, INS, U,
DIV.
*/

enum ListStyle {
    OL,
    UL,
    ListStyle_Max
};

class HTMLReader
{
private:
	QString href;
	QString extLinks;
	int extIndex;
	gtWriter *writer;
	gtParagraphStyle *pstyle;

    QMap<QString, Alignment> alignments;
    QMap<QString, QString> *elements;
    QVector<QString> *blockElements;
    QStack<gtParagraphStyle *> *styleStack;
    int styleNumber;
    /* This indicates that one line break has just been done.
     * Without this, two following block elements would create
     * three blocks instead of two (one block would be empty) */
    bool lineBreak;

    /* This indicates that line break just occured. This is to
     * filter spaces on the beginning of lines after line breaks */
    bool afterBreak;

	QStack<ListStyle> listStyles;
	QStack<int> nextItemNumbers;

    struct {
        bool li;
        bool pre;
    } inElement;

	bool noFormatting;

	void initPStyles();
    double getSize(double currentFontSize, QString styleValue, bool isFontSize);
	static HTMLReader* hreader;
public:
	HTMLReader(gtParagraphStyle *ps, gtWriter *w, bool textOnly);
	~HTMLReader();
	void parse(QString filename);
	static void startElement(void *user_data, const xmlChar * fullname, const xmlChar ** atts);
	static void endElement(void *user_data, const xmlChar * name);
	static void characters(void *user_data, const xmlChar * ch, int len);
	bool startElement(const QString&, const QString&, const QString &name, const QXmlAttributes &attrs);
	bool endElement(const QString&, const QString&, const QString &name);
	bool characters(const QString &ch);
    bool setStyle(gtParagraphStyle *style, const QString &css);
};

#endif
