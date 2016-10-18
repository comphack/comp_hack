/**
 * @file tools/capgrep/src/HexView.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Widget to display a hex dump.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOOLS_CAPGREP_SRC_HEXVIEW_H
#define TOOLS_CAPGREP_SRC_HEXVIEW_H

#include <PushIgnore.h>
#include <QByteArray>

#include <QPen>
#include <QFont>
#include <QBrush>
#include <QWidget>
#include <PopIgnore.h>

/**
 * Current state while rendering the @ref HexView. This state changes as each
 * line of bytes is rendered and does not persist. This data was split into a
 * separate class for this reason. Some of the data seems static but could
 * change if a new font was set.
 */
class HexViewPaintState
{
public:
    /// Width of the address.
    int addrWidth;

    /// Width of a byte.
    int byteWidth;

    /// Width of a single character.
    int charWidth;

    /// Minimum width of the widget.
    int minWidth;

    /// Start of the binary data (in hex).
    int binaryX;

    /// Height of a line.
    int lineHeight;
    int ascent;
    int x, y;

    int asciiLine;
    int asciiX;

    int endLine;

    /// How many bytes are on a single line.
    int bytesPerLine;

    int sz;
};

class HexView : public QWidget
{
    Q_OBJECT

public:
    HexView(QWidget *parent = 0);

    int pointToAddr(const QPoint& pt, bool *isAscii = 0) const;

    int startOffset() const;
    int stopOffset() const;

signals:
    void selectionChanged();

public slots:
    void setData(const QByteArray& d);
    void setSelection(int start, int stop);
    void scrollToOffset(int offset);

protected:
    virtual void paintEvent(QPaintEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    void initState(HexViewPaintState& state) const;
    void paintLine(const HexViewPaintState& state, QPainter& p, int addr);

    QMargins mMargin;
    QMargins mLinePadding;
    QMargins mAddrPadding;
    QMargins mBytePadding;
    QMargins mBytesPadding;
    QMargins mAsciiPadding;
    QMargins mCharPadding;

    int mByteSpacing;
    int mCharSpacing;

    int mSelectionStart;
    int mSelectionEnd;

    bool mSelectionActive;

    QFont  mFont;
    QPen   mFontColor;
    QPen   mAltFontColor;
    QBrush mBackground;
    QBrush mAddrBackground;

    QByteArray mData;
};

#endif // TOOLS_CAPGREP_SRC_HEXVIEW_H
