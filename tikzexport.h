/*  Copyright (c) 2012-2013, Dominik Haumann <dhaumann@kde.org>
    All rights reserved.

    License: FreeBSD License

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef QT_TIKZ_PICTURE_H
#define QT_TIKZ_PICTURE_H

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVector>

class QTextStream;
class QColor;
class QPointF;
class QRectF;
class QPainterPath;

class QTikzPicture
{
    public:
        QTikzPicture();

        void setStream(QTextStream* textStream, int precision = 2);

        QString registerColor(const QColor& color);

        void begin(const QString& options = QString());
        void end();

        void beginScope(const QString& options = QString());
        void endScope();

        void newline(int count = 1);
        void comment(const QString& text);

        void path(const QPainterPath& path, const QString& options = QString());
        void path(const QRectF& rect, const QString& options = QString());

        void clip(const QPainterPath& path);
        void clip(const QRectF& rect);

        void circle(const QPointF& center, qreal radius, const QString& options = QString());

        void line(const QPointF& p, const QPointF& q, const QString& options = QString());
        void line(const QVector<QPointF>& points, const QString& options = QString());

        QTikzPicture& operator<< (const QString& text);
        QTikzPicture& operator<< (const char* text);
        QTikzPicture& operator<< (double number);
        QTikzPicture& operator<< (int number);

    private:
        QTextStream* ts;
        QHash<QString, bool> m_colors;
};

#endif // QT_TIKZ_PICTURE_H

// kate: replace-tabs on; indent-width 4;
