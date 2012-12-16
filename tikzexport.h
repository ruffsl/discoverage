#ifndef TIKZ_EXPORT_H
#define TIKZ_EXPORT_H

#include <QtCore/QVector>
#include <QtGui/QPainterPath>
#include <QtCore/QTextStream>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtGui/QPolygonF>
#include <QtGui/QColor>

/**
 * Helper namespace to export as tikz picture.
 */
namespace tikz
{
    /** begin tikz picture */
    extern void begin(QTextStream& ts, qreal scale);

    /** end tikz picture */
    extern void end(QTextStream& ts);

    /** begin a own tikz scope */
    extern void beginScope(QTextStream& ts);

    /** end a tikz scope */
    extern void endScope(QTextStream& ts);

    /** insert a blank line */
    extern void newline(QTextStream& ts);

    /** Export QPainterPath. */
    extern void path(QTextStream& ts, const QPainterPath& path);

    /** Export QPolygon lines. */
    extern void lines(QTextStream& ts, const QPolygonF& polygon);

    /** Set clip path. */
    extern void clip(QTextStream& ts, const QPainterPath& path);

    /** Export Circle. */
    extern void circle(QTextStream& ts, const QPointF& center, qreal radius);

    /** Export Line. */
    extern void line(QTextStream& ts, const QPointF& p, const QPointF& q);

    /** Export arrow. */
    extern void arrow(QTextStream& ts, const QPointF& p, const QPointF& q);

    /** Fill rectangle. */
    extern void fill(QTextStream& ts, const QRectF& rect, const QColor& color);
};

#endif // TIKZ_EXPORT_H
