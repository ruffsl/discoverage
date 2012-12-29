#ifndef TIKZ_EXPORT_H
#define TIKZ_EXPORT_H

#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtGui/QPainterPath>
#include <QtCore/QTextStream>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtGui/QPolygonF>
#include <QtGui/QColor>

class QTikzPicture
{
    public:
        QTikzPicture();

        void setStream(QTextStream* textStream);

        QString registerColor(const QColor& color);

        void begin(const QString& options = QString());
        void end();

        void beginScope(const QString& options = QString());
        void endScope();

        void newline();
        void comment(const QString& text);

        void path(const QPainterPath& path, const QString& options = QString());
        void path(const QRectF& rect, const QString& options = QString());

        void clip(const QPainterPath& path);
        void clip(const QRectF& rect);

        void circle(const QPointF& center, qreal radius, const QString& options = QString());

        void line(const QPointF& p, const QPointF& q, const QString& options = QString());

    private:
        QTextStream* ts;
        QHash<QString, bool> m_colors;
};

/**
 * Helper namespace to export as tikz picture.
 */
namespace tikz
{
    /** begin tikz picture */
    extern void begin(QTextStream& ts, const QString& options = QString());

    /** end tikz picture */
    extern void end(QTextStream& ts);

    /** begin a own tikz scope */
    extern void beginScope(QTextStream& ts, const QString& options = QString());

    /** end a tikz scope */
    extern void endScope(QTextStream& ts);

    /** insert a blank line */
    extern void newline(QTextStream& ts);

    /** Export QPainterPath. */
    extern void path(QTextStream& ts, const QPainterPath& path, const QString& options = QString());
    extern void path(QTextStream& ts, const QRectF& rect, const QString& options = QString());

    /** Export QPolygon lines. */
    extern void lines(QTextStream& ts, const QPolygonF& polygon);

    /** Set clip path. */
    extern void clip(QTextStream& ts, const QPainterPath& path);

    /** Set clip rect. */
    extern void clip(QTextStream& ts, const QRectF& rect);

    /** Export Circle. */
    extern void circle(QTextStream& ts, const QPointF& center, qreal radius, const QString& options = QString());

    /** Export Line. */
    extern void line(QTextStream& ts, const QPointF& p, const QPointF& q);

    /** Export arrow. */
    extern void arrow(QTextStream& ts, const QPointF& p, const QPointF& q);

    /** Fill rectangle. */
    extern void fill(QTextStream& ts, const QRectF& rect, const QColor& brush);
    extern void filldraw(QTextStream& ts, const QRectF& rect, const QColor& brush, const QColor& pen);
    extern void drawRect(QTextStream& ts, const QRectF& rect, const QColor& pen);

    /** QString pendents (named colors) */
    extern void fill(QTextStream& ts, const QRectF& rect, const QString& brush);
    extern void filldraw(QTextStream& ts, const QRectF& rect, const QString& brush, const QString& pen);
    extern void drawRect(QTextStream& ts, const QRectF& rect, const QString& pen);

    /** generate a uniq color identifier*/
    extern QString uniqColorString();
};

#endif // TIKZ_EXPORT_H

// kate: replace-tabs on; indent-width 4;
