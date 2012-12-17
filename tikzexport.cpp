#include "tikzexport.h"

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
        .arg(brush.redF()).arg(brush.greenF()).arg(brush.blueF());

    ts << brushColor;

    fill(ts, rect, "col" + uniqBrushColor);
}

void filldraw(QTextStream& ts, const QRectF& rect, const QColor& brush, const QColor& pen)
{
    const QString uniqBrushColor = uniqColorString();
    const QString uniqPenColor = uniqColorString();

    QString brushColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqBrushColor)
        .arg(brush.redF()).arg(brush.greenF()).arg(brush.blueF());

    QString penColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqPenColor)
        .arg(pen.redF()).arg(pen.greenF()).arg(pen.blueF());

    ts << brushColor << penColor;

    filldraw(ts, rect, "col" + uniqBrushColor, "col" + uniqPenColor);
}

void drawRect(QTextStream& ts, const QRectF& rect, const QColor& pen)
{
    const QString uniqPenColor = uniqColorString();

    QString penColor = QString("\\definecolor{col%1}{rgb}{%2, %3, %4}\n").arg(uniqPenColor)
        .arg(pen.redF()).arg(pen.greenF()).arg(pen.blueF());

    ts << penColor;

    drawRect(ts, rect, "col" + uniqPenColor);
}

void fill(QTextStream& ts, const QRectF& rect, const QString& brush)
{
    ts << QString("\\filldraw[fill=%1, draw=%1] (%2, %3) rectangle (%4, %5);\n")
        .arg(brush)
        .arg(rect.left()).arg(rect.bottom()).arg(rect.right()).arg(rect.top());
}

void filldraw(QTextStream& ts, const QRectF& rect, const QString& brush, const QString& pen)
{
    ts << QString("\\filldraw[fill=%1, draw=%2] (%3, %4) rectangle (%5, %6);\n")
        .arg(brush)
        .arg(pen)
        .arg(rect.left()).arg(rect.bottom()).arg(rect.right()).arg(rect.top());
}

void drawRect(QTextStream& ts, const QRectF& rect, const QString& pen)
{
    ts << QString("\\draw[%1] (%2, %3) rectangle (%4, %5);\n")
        .arg(pen)
        .arg(rect.left()).arg(rect.bottom()).arg(rect.right()).arg(rect.top());
}

}

// kate: replace-tabs on; indent-width 4;
