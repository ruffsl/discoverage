#include "tikzexport.h"

namespace tikz
{

void begin(QTextStream& ts, qreal scale)
{
    QString args = QString("scale=%1").arg(scale);
    ts << QString("\\begin{tikzpicture}[%1]\n").arg(args);
}

void end(QTextStream& ts)
{
    ts << "\\end{tikzpicture}\n";
}

void beginScope(QTextStream& ts)
{
    ts << "\\begin{scope}\n";
}

void endScope(QTextStream& ts)
{
    ts << "\\end{scope}\n";
}

void newline(QTextStream& ts)
{
    ts << "\n";
}

void path(QTextStream& ts, const QPainterPath& path)
{
    int i = 0;
    for (i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                ts << " -- cycle;\n";
            }
            ts << QString("\\draw (%1, %2)").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
        } else if (element.type == QPainterPath::LineToElement) {
            ts << QString(" -- (%1, %2)").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
        }
    }
    if (i > 0) {
        ts << " -- cycle;\n";
    }
}

void lines(QTextStream& ts, const QPolygonF& polygon)
{
    if (polygon.size() == 0) {
        return;
    }

    ts << "\\draw ";

    for (int i = 0; i < polygon.size(); ++i) {
        if (i > 0) {
            ts << " -- ";
        }
        const QPointF& p = polygon[i];
        ts << QString("(%1, %2)").arg(p.x(), 0, 'f').arg(p.y(), 0, 'f');
    }

    ts << ";\n";
}

void clip(QTextStream& ts, const QPainterPath& path)
{
    int i = 0;
    for (i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                ts << " -- cycle;\n";
            }
            ts << QString("\\clip (%1, %2)").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
        } else if (element.type == QPainterPath::LineToElement) {
            ts << QString(" -- (%1, %2)").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
        }
    }
    if (i > 0) {
        ts << " -- cycle;\n";
    }
}

void circle(QTextStream& ts, const QPointF& center, qreal radius)
{
    ts << QString("\\draw (%1, %2) circle (%3cm);\n").arg(center.x()).arg(center.y()).arg(radius);
}

void line(QTextStream& ts, const QPointF& p, const QPointF& q)
{
    ts << QString("\\draw (%1, %2) -- (%3, %4);\n").arg(p.x()).arg(p.y()).arg(q.x()).arg(q.y());
}

void arrow(QTextStream& ts, const QPointF& p, const QPointF& q)
{
    ts << QString("\\draw[->] (%1, %2) -- (%3, %4);\n").arg(p.x()).arg(p.y()).arg(q.x()).arg(q.y());
}

void fill(QTextStream& ts, const QRectF& rect, const QColor& color)
{
    // lame: construct unique string without numbers for the color
    static int i = 0;
    ++i;
    QString col = QString::number(i);
    for (int i = 0; i < col.length(); ++i) {
        col[i] = col[i].toAscii() + (char)('A' - '0');
    }
    ts << QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(col)
        .arg(color.redF()).arg(color.greenF()).arg(color.blueF());

    ts << QString("\\filldraw[fill=col%1, draw=col%1] (%2, %3) rectangle (%4, %5);\n").arg(col)
        .arg(rect.left()).arg(rect.bottom()).arg(rect.right()).arg(rect.top());
}

}
