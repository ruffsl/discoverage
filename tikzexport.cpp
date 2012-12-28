#include "tikzexport.h"

#include <QDebug>

namespace tikz
{

void begin(QTextStream& ts, const QString& options)
{
    if (options.isEmpty()) {
        ts << "\\begin{tikzpicture}\n";
    } else {
        ts << "\\begin{tikzpicture}[" << options << "]\n";
    }
}

void end(QTextStream& ts)
{
    ts << "\\end{tikzpicture}\n";
}

void beginScope(QTextStream& ts, const QString& options)
{
    if (options.isEmpty()) {
        ts << "\\begin{scope}\n";
    } else {
        ts << "\\begin{scope}[" << options << "]\n";
    }
}

void endScope(QTextStream& ts)
{
    ts << "\\end{scope}\n";
}

void newline(QTextStream& ts)
{
    ts << "\n";
}

void path(QTextStream& ts, const QPainterPath& path, const QString& options)
{
    int i = 0;
    for (i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                ts << " -- cycle;\n";
            }
            ts << "\\draw[" << options << "] (" << element.x << ", " << element.y << ")";
        } else if (element.type == QPainterPath::LineToElement) {
            ts << " -- (" << element.x << ", " << element.y << ")";
        }
    }
    if (i > 0) {
        ts << " -- cycle;\n";
    }
}

void path(QTextStream& ts, const QRectF& rect, const QString& options)
{
    if (options.isEmpty()) {
        ts << "\\path ";
    } else {
        ts << "\\path[" << options << "] ";
    }

    ts << "(" << rect.left() << ", " << rect.top()
       << ") rectangle (" << rect.right() << ", " << rect.bottom() << ");\n";
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
        ts << "(" << p.x() << ", " << p.y() << ")";
    }

    ts << ";\n";
}

void clip(QTextStream& ts, const QPainterPath& path)
{
    if (path.elementCount() == 0)
        return;

    ts << "\\clip ";

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                ts << " -- cycle";
                ts << "\n      ";
            }
        } else if (element.type == QPainterPath::LineToElement) {
            ts << " -- ";
        } else {
            qDebug() << "tikz::clip: uknown QPainterPath segment type";
        }
        ts << "(" << element.x << ", " << element.y << ")";
    }

    ts << " -- cycle;\n";
}

void clip(QTextStream& ts, const QRectF& rect)
{
    ts << "\\clip (" << rect.left() << ", " << rect.top()
       << ") rectangle (" << rect.right() << ", " << rect.bottom() << ");";
}

void circle(QTextStream& ts, const QPointF& center, qreal radius, const QString& options)
{
    if (options.isEmpty()) {
        ts << "\\draw (" << center.x() << ", " << center.y() << ") circle (" << radius << "cm);\n";
    } else {
        ts << "\\draw[" << options << "] (" << center.x() << ", " << center.y() << ") circle (" << radius << "cm);\n";
    }
}

void line(QTextStream& ts, const QPointF& p, const QPointF& q)
{
    ts << "\\draw (" << p.x() << ", " << p.y() << ") -- (" << q.x() << ", " << q.y() << ");\n";
}

void arrow(QTextStream& ts, const QPointF& p, const QPointF& q)
{
    ts << "\\draw[->] (" << p.x() << ", " << p.y() << ") -- (" << q.x() << ", " << q.y() << ");\n";
}

QString uniqColorString()
{
    // lame: construct unique string without numbers for the color
    static int i = 0;
    ++i;
    QString colorName = QString::number(i);
    for (int i = 0; i < colorName.length(); ++i) {
        colorName[i] = colorName[i].toAscii() + (char)('A' - '0');
    }

    return colorName;
}

void fill(QTextStream& ts, const QRectF& rect, const QColor& brush)
{
    const QString uniqBrushColor = uniqColorString();

    QString brushColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqBrushColor)
        .arg(brush.redF(), 0, 'f').arg(brush.greenF(), 0, 'f').arg(brush.blueF(), 0, 'f');

    ts << brushColor;

    fill(ts, rect, "col" + uniqBrushColor);
}

void filldraw(QTextStream& ts, const QRectF& rect, const QColor& brush, const QColor& pen)
{
    const QString uniqBrushColor = uniqColorString();
    const QString uniqPenColor = uniqColorString();

    QString brushColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqBrushColor)
        .arg(brush.redF(), 0, 'f').arg(brush.greenF(), 0, 'f').arg(brush.blueF(), 0, 'f');

    QString penColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqPenColor)
        .arg(pen.redF(), 0, 'f').arg(pen.greenF(), 0, 'f').arg(pen.blueF(), 0, 'f');

    ts << brushColor << penColor;

    filldraw(ts, rect, "col" + uniqBrushColor, "col" + uniqPenColor);
}

void drawRect(QTextStream& ts, const QRectF& rect, const QColor& pen)
{
    const QString uniqPenColor = uniqColorString();

    QString penColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqPenColor)
        .arg(pen.redF(), 0, 'f').arg(pen.greenF(), 0, 'f').arg(pen.blueF(), 0, 'f');

    ts << penColor;

    drawRect(ts, rect, "col" + uniqPenColor);
}

void fill(QTextStream& ts, const QRectF& rect, const QString& brush)
{
    ts << "\\filldraw[fill=" << brush << ", draw=" << brush
       << "] (" << rect.left() << ", " << rect.bottom() << ") rectangle ("
       << rect.right() << ", " << rect.top() << ");\n";
}

void filldraw(QTextStream& ts, const QRectF& rect, const QString& brush, const QString& pen)
{
    ts << "\\filldraw[fill=" << brush << ", draw=" << pen
       << "] (" << rect.left() << ", " << rect.bottom() << ") rectangle ("
       << rect.right() << ", " << rect.top() << ");\n";
}

void drawRect(QTextStream& ts, const QRectF& rect, const QString& pen)
{
    ts << "\\draw[" << pen << "] (" << rect.left() << ", " << rect.bottom()
       << ") rectangle (" << rect.right() << ", " << rect.top() << ");\n";
}

}

// kate: replace-tabs on; indent-width 4;
