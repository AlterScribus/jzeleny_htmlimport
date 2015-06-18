/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
 *   Copyright (C) 2012 by Jan Zeleny                                      *
 *   jz@janzeleny.cz                                                       *
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

#include <QObject>
#include <QByteArray>
#include <QMessageBox>
#include "htmlreader.h"

#include "scribusstructs.h"
#include "scribusdoc.h"
#include "gtmeasure.h"

HTMLReader* HTMLReader::hreader = NULL;

extern htmlSAXHandlerPtr mySAXHandler;

HTMLReader::HTMLReader(gtParagraphStyle *ps, gtWriter *w, bool textOnly):
href(""), extLinks(""), extIndex(1), writer(w), pstyle(ps), lineBreak(true),
afterBreak(true)
{
    inElement.li = false;
    inElement.pre = false;

    noFormatting = textOnly;
    hreader = this;

    alignments.insert(QString("left"), LEFT);
    alignments.insert(QString("center"), CENTER);
    alignments.insert(QString("right"), RIGHT);
    alignments.insert(QString("justify"), BLOCK);
    initPStyles();
}

void HTMLReader::initPStyles()
{
    QString style;
    QString element;
    const char *block[] = {"center", "p", "ul", "ol", "li", "pre", "div", "h1",
                           "h2", "h3", "h4", "h5", "h6", NULL};

    styleStack = new QStack<gtParagraphStyle *>();
    styleStack->push(pstyle);
    styleNumber = 0;

    blockElements = new QVector<QString>();
    for (int i = 0; block[i] != NULL; i++) {
        element = QString(block[i]);
        blockElements->append(element);
    }

    elements = new QMap<QString, QString>();
    style = QString("font-size: +2.5px; font-weight: bold; margin-top: 2.5px; margin-bottom: 1.25px");
    element = QString("h6");
    elements->insert(element, style);

    style = QString("font-size: +5px; font-weight: bold; margin-top: 5px; margin-bottom: 2.5px");
    element = QString("h5");
    elements->insert(element, style);

    style = QString("font-size: +10px; font-weight: bold; margin-top: 10px; margin-bottom: 5px");
    element = QString("h4");
    elements->insert(element, style);

    style = QString("font-size: +20px; font-weight: bold; margin-top: 20px; margin-bottom: 10px");
    element = QString("h3");
    elements->insert(element, style);

    style = QString("font-size: +40px; font-weight: bold; margin-top: 30px; margin-bottom: 20px");
    element = QString("h2");
    elements->insert(element, style);

    style = QString("font-size: +60px; font-weight: bold; margin-top: 40px; margin-bottom: 30px");
    element = QString("h1");
    elements->insert(element, style);

    style = QString("font-family: Courier Regular");
    element = QString("code");
    elements->insert(element, style);

    style = QString("margin-bottom: %1").arg(gtMeasure::i2d(5, SC_MM));
    element = QString("p");
    elements->insert(element, style);

    style = QString("font-style: italic");
    element = QString("em");
    elements->insert(element, style);
    element = QString("i");
    elements->insert(element, style);

    style = QString("font-weight: bold");
    element = QString("strong");
    elements->insert(element, style);
    element = QString("b");
    elements->insert(element, style);

    style = QString("text-align: center");
    element = QString("center");
    elements->insert(element, style);

    style = QString("color: Blue; text-decoration: underline");
    element = QString("a");
    elements->insert(element, style);

    style = QString("text-decoration: underline");
    element = QString("ins");
    elements->insert(element, style);
    element = QString("u");
    elements->insert(element, style);

    style = QString("text-decoration: strikethrough");
    element = QString("del");
    elements->insert(element, style);

    style = QString("margin-left: +25px");
    element = QString("ul");
    elements->insert(element, style);
    element = QString("ol");
    elements->insert(element, style);
}

void HTMLReader::startElement(void*, const xmlChar * fullname, const xmlChar ** atts)
{
    QString name(QString((const char*) fullname).toLower());
    QXmlAttributes attrs;
    if (atts)
    {
        for(const xmlChar** cur = atts; cur && *cur; cur += 2)
            attrs.append(QString((char*)*cur), NULL, QString((char*)*cur), QString((char*)*(cur + 1)));
    }
    hreader->startElement(NULL, NULL, name, attrs);
}

bool HTMLReader::startElement(const QString&, const QString&, const QString &name, const QXmlAttributes &attrs) 
{
    int i;
    gtParagraphStyle *newStyle;
    QString style;
    QString styleAttr("");

    newStyle = new gtParagraphStyle(*(styleStack->top()));
    if (name == "br") {
        writer->append(QString(SpecialChars::LINEBREAK));
        afterBreak = true;
        delete newStyle;
        return true;
    } else if (name == "pre") {
        inElement.pre = true;
    } else if (name == "a") {
        if (!href.isEmpty()) {
            /* Already within another link */
            return false;
        }

        for (i = 0; i < attrs.count(); i++) {
            if (attrs.localName(i) == "href") {
                href = attrs.value(i);
                break;
            }
        }
        if (href.isEmpty()) {
            /* This was not correct link, mark it for end tag handler and
             * stop further processing (i.e. don't push the new style)
             */
            return true;
        }
    } else if (name == "img") {
        QString imgline("(img,");
        for (int i = 0; i < attrs.count(); i++)
        {
            if (attrs.localName(i) == "src")
            {
                if (attrs.value(i).indexOf("data:image") == -1) {
                    imgline +=  " src: " + attrs.value(i);
                } else {
                    imgline +=  " src: embedded image";
                }
            }
            if (attrs.localName(i) == "alt")
            {
                if (!attrs.value(i).isEmpty())
                    imgline += ", alt: " + attrs.value(i);
            }
        }
        imgline += ")\n\n";
        writer->append(imgline, pstyle);
    } else if (name == "ul") {
        listStyles.push(UL);
        /* We don't need numbers, push zero */
        nextItemNumbers.push(0);
    } else if (name == "ol") {
        listStyles.push(OL);
        nextItemNumbers.push(1);
    } else if (name == "li") {
        inElement.li = true;
    } else if (name == "sub") {
        if (!newStyle->getFont()->isToggled(SUBSCRIPT)) {
            newStyle->getFont()->toggleEffect(SUBSCRIPT);
        }
    } else if (name == "sup") {
        if (!newStyle->getFont()->isToggled(SUPERSCRIPT)) {
            newStyle->getFont()->toggleEffect(SUPERSCRIPT);
        }
    }

    if (name != "li" && lineBreak == false) {
        for (int i = 0; i < blockElements->count(); i++) {
            if (name == blockElements->at(i)) {
                writer->append("\n");
                afterBreak = true;
                break;
            }
        }
    }
    lineBreak = false;

    for (i = 0; i < attrs.count(); i++) {
        if (attrs.localName(i) == "style") {
            /* If, by accident, there are two or more definitions,
             * we want the last one
             */
            styleAttr = attrs.value(i);
        }
    }

    if (elements->contains(name)) {
        style = elements->value(name);
    } else {
        style = "";
    }

    if (!styleAttr.isEmpty()) {
        style += ";"+styleAttr;
    }

    if (setStyle(newStyle, style)) {
        styleNumber++;
        newStyle->setName(QString("style")+QString::number(styleNumber));
        styleStack->push(newStyle);
        writer->setParagraphStyle(newStyle);
    } else {
        /* An error occurred */
    }

    return true;
}
void HTMLReader::characters(void*, const xmlChar * ch, int len)
{
    QString chars = QString::fromUtf8((const char*) ch, len);
    hreader->characters(chars);
}

bool HTMLReader::characters(const QString &ch) 
{
    QString text = ch;
    int number;
    bool space_before = false;
    bool space_after = false;

    if (text.isEmpty()) {
        return true;
    }

    /* Correctly remove all newlines */
    if (!inElement.pre) {
        if (text[0].isSpace() && !afterBreak) space_before = true;
        if (text[text.size()-1].isSpace()) space_after = true;

        /* This replaces al white spaces with spaces and collapses
         * more spaces into one. The last thing is that it trims
         * the string */
        text = text.simplified();

        /* Now we need to put trimmed spaces back */
        if (text.isEmpty()) {
            if ((space_before || space_after) && !afterBreak) text += " ";
            else return true;
        } else {
            if (space_before) text = " "+text;
            if (space_after) text += " ";
        }
    } else {
        /* Filter leading newline in the "pre" element */
        while (text[0] == '\n' || text[0] == '\r') {
            text = text.right(text.length()-1);
        }
    }

    if (inElement.li) {
        if (listStyles.top() == UL) {
            text = "- " + text;
        } else {
            number = nextItemNumbers.pop();
            text = QString("%1. ").arg(number) + text;
            nextItemNumbers.push(number+1);
        }
    }

    if (noFormatting) {
        writer->appendUnstyled(text);
    } else {
        //writer->append(text, styleStack->top(), true);
        writer->append(text, styleStack->top());
    }
    afterBreak = false;

    return true;
}

void HTMLReader::endElement(void*, const xmlChar * name)
{
    QString nname(QString((const char*) name).toLower());
    hreader->endElement(NULL, NULL, nname);
}

bool HTMLReader::endElement(const QString&, const QString&, const QString &name)
{
    for (int i = 0; i < blockElements->count(); i++) {
        if (name == blockElements->at(i)) {
            writer->append("\n");
            lineBreak = true;
            afterBreak = true;
            break;
        }
    }

    if (name == "ul" || name == "ol") {
        if (listStyles.count()) {
            listStyles.pop();
            nextItemNumbers.pop();
        }
    } else if (name == "li") {
        inElement.li = false;
    } else if (name == "a" && href.isEmpty()) {
        /* This was not valid link, no style has been pushed,
         * don't pop it now
         */
        return true;
    } else if (name == "pre") {
        inElement.pre = false;
    } else if (name == "br") {
        afterBreak = true;
        return true;
    }

    if (styleStack->count() > 1) {
        styleStack->pop();
    }

    if (name == "a") {
        if (!href.isEmpty() &&
            (href.indexOf("//") != -1 ||
             href.indexOf("mailto:") != -1 ||
             href.indexOf("www") != -1))
        {
            href = href.remove("mailto:");
            writer->append(QString(" [%1]").arg(extIndex), pstyle);
            extLinks += QString("[%1] ").arg(extIndex) + href + "\n";
            extIndex++;
        }
        href = "";
    }

    writer->setParagraphStyle(styleStack->top());

    return true;
}

void HTMLReader::parse(QString filename)
{
#if defined(_WIN32)
    QString fname = QDir::toNativeSeparators(filename);
    QByteArray fn = (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based) ? fname.toUtf8() : fname.toLocal8Bit();
#else
    QByteArray fn(filename.toLocal8Bit());
#endif
    htmlSAXParseFile(fn.data(), NULL, mySAXHandler, NULL);
}

bool HTMLReader::setStyle(gtParagraphStyle *style, const QString &css)
{
    QStringList styleItem;
    QString styleAttr;
    QString styleValue;
    bool ret = true;
    double size = 0.0;

    gtFont *font = style->getFont();
    /*
    QMessageBox::information(writer->getPageItem()->doc()->WinHan,
                             QString("warning"),
                             css, QMessageBox::Ok);
    */
    QStringList styles = css.split(QChar(';'), QString::SkipEmptyParts,
                                   Qt::CaseSensitive);

    for (int i = 0; i < styles.size(); i++)
    {
        styleItem = styles.at(i).split(QChar(':'), QString::KeepEmptyParts,
                                       Qt::CaseSensitive);
        if (styleItem.size() != 2) {
            /* This can happen e.g. when element with pre-defined style
             * contains style attribute */
            continue;
        }

        styleAttr = styleItem.at(0).trimmed();
        styleValue = styleItem.at(1).trimmed();

        if (styleAttr == "margin-top") {
            size = getSize(font->getSize(), styleValue, false);

            if (size >= 0.0) {
                style->setSpaceAbove(size);
            } else {
                ret = false;
            }
        } else if (styleAttr == "margin-bottom") {
            size = getSize(font->getSize(), styleValue, false);

            if (size >= 0.0) {
                style->setSpaceBelow(size);
            } else {
                ret = false;
            }
        } else if (styleAttr == "margin-left") {
            size = getSize(font->getSize(), styleValue, false);

            if (size >= 0.0) {
                style->setIndent(size);
            } else {
                ret = false;
            }
        } else if (styleAttr == "font-weight") {
            if (styleValue == "bold") {
                font->setWeight(BOLD);
            } else if (styleValue == "normal") {
                font->setWeight(NO_WEIGHT);
            } else {
                ret = false;
            }
        } else if (styleAttr == "font-style") {
            if (styleValue == "italic") {
                font->setSlant(ITALIC);
            } else if (styleValue == "normal") {
                font->setSlant(NO_SLANT);
            } else {
                ret = false;
            }
        } else if (styleAttr == "font-size") {
            size = getSize(font->getSize(), styleValue, true);

            if (size >= 0.0) {
                font->setSize(size);
            } else {
                ret = false;
            }
        } else if (styleAttr == "font-family") {
            if (styleValue.contains(QChar(','))) {
                ret = false;
                continue;
            }
            font->setName(styleValue);
        } else if (styleAttr == "color") {
            /*
            if (!writer->getPageItem()->doc()->PageColors.contains(newColor)) {
                ret = false;
                continue;
            }
            */
            font->setColor(styleValue);
        } else if (styleAttr == "background-color") {
            /* TODO: detect "none" */
            /* TODO: probably use font->toggleEffect(OUTLINE) */
            //font->setStrokeColor();
        } else if (styleAttr == "text-align") {
            Alignment al = alignments.value(styleValue);
            style->setAlignment(al);
        } else if (styleAttr == "text-decoration") {
            if (styleValue == "none") {
                if (font->isToggled(UNDERLINE)) {
                    font->toggleEffect(UNDERLINE);
                }
                if (font->isToggled(STRIKETHROUGH)) {
                    font->toggleEffect(STRIKETHROUGH);
                }
            } else if (styleValue == "underline") {
                if (!font->isToggled(UNDERLINE)) {
                    font->toggleEffect(UNDERLINE);
                }
            } else if (styleValue == "strikethrough") {
                if (!font->isToggled(STRIKETHROUGH)) {
                    font->toggleEffect(STRIKETHROUGH);
                }
            }
        } else {
            ret = false;
        }
    }
    return ret;
}

double HTMLReader::getSize(double currentFontSize, QString styleValue, bool isFontSize)
{
    bool addition = false;
    bool conversionOk;
    double ret;

    QString px = QString("px");
    QString pt = QString("pt");
    QString percent = QString("%");
    QString plus = QString("+");
    QString em = QString("em");

    /* Font size is internally stored as number multiplied by 10 */
    currentFontSize /= 10;

    if (styleValue.startsWith(plus)) {
        styleValue.remove(0, 1);
        addition = true;
    }

    if (styleValue.endsWith(em, Qt::CaseInsensitive)) {
        /* em */
        styleValue = styleValue.remove(em, Qt::CaseInsensitive);
        ret = styleValue.toDouble(&conversionOk);

        if (conversionOk) {
            ret *= currentFontSize;
        }
    } else if (styleValue.endsWith(percent, Qt::CaseInsensitive)) {
        /* % */
        styleValue = styleValue.remove(percent, Qt::CaseInsensitive);
        ret = styleValue.toDouble(&conversionOk);

        if (conversionOk) {
            ret *= currentFontSize/100;
        }
    } else {
        /* px/pt */
        if (styleValue.endsWith(px, Qt::CaseInsensitive)) {
            styleValue = styleValue.remove(px, Qt::CaseInsensitive);
        } else if (styleValue.endsWith(pt, Qt::CaseInsensitive)) {
            styleValue = styleValue.remove(pt, Qt::CaseInsensitive);
        }

        ret = styleValue.toDouble(&conversionOk);
    }

    if (!conversionOk) {
        return -1.0;
    }

    if (isFontSize) {
        /* This size will be used for font, multiply it by 10 */
        ret *= 10;
    }

    if (addition) {
        /* Restore original value of currentFontSize */
        currentFontSize *= 10;
        return ret+currentFontSize;
    } else {
        return ret;
    }
}

htmlSAXHandler mySAXHandlerStruct = {
    NULL, // internalSubset,
    NULL, // isStandalone,
    NULL, // hasInternalSubset,
    NULL, // hasExternalSubset,
    NULL, // resolveEntity,
    NULL, // getEntity,
    NULL, // entityDecl,
    NULL, // notationDecl,
    NULL, // attributeDecl,
    NULL, // elementDecl,
    NULL, // unparsedEntityDecl,
    NULL, // setDocumentLocator,
    NULL, // startDocument,
    NULL, // endDocument,
    HTMLReader::startElement,
    HTMLReader::endElement,
    NULL, // reference,
    HTMLReader::characters,
    NULL, // ignorableWhitespace,
    NULL, // processingInstruction,
    NULL, // comment,
    NULL, // warning,
    NULL, // error,
    NULL, // fatalError,
    NULL, // getParameterEntity,
    NULL, // cdata,
    NULL,
    1
#ifdef HAVE_XML26
    ,
    NULL,
    NULL,
    NULL,
    NULL
#endif
};

htmlSAXHandlerPtr mySAXHandler = &mySAXHandlerStruct;

HTMLReader::~HTMLReader()
{
    gtParagraphStyle *style;
    QString styleString;

    if (!extLinks.isEmpty()) {
        style = new gtParagraphStyle(*pstyle);
        styleString = elements->value(QString("h4"));
        setStyle(style, styleString);
        writer->append(QObject::tr("\nExternal Links\n"), style);
        writer->append(extLinks, pstyle);
        delete style;
    }

    delete elements;
    delete blockElements;
    /* We don't want to delete pstyle -> start from 1 */
    for (int i = 1; i < styleStack->count(); i++) {
        style = styleStack->pop();
        delete style;
    }
    delete styleStack;
}

