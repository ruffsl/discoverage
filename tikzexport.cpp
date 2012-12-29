#include "tikzexport.h"

#include <QDebug>

QTikzPicture::QTikzPicture()
    : ts(0)
{
}

void QTikzPicture::setStream(QTextStream* textStream)
{
    ts = textStream;
}

QString QTikzPicture::registerColor(const QColor& color)
{
    QString name = color.name();
    if (name.startsWith('#')) name.remove(0, 1);

    name.replace("0", "P", Qt::CaseInsensitive);
    name.replace("1", "Q", Qt::CaseInsensitive);
    name.replace("2", "W", Qt::CaseInsensitive);
    name.replace("3", "E", Qt::CaseInsensitive);
    name.replace("4", "R", Qt::CaseInsensitive);
    name.replace("5", "T", Qt::CaseInsensitive);
    name.replace("6", "Z", Qt::CaseInsensitive);
    name.replace("7", "U", Qt::CaseInsensitive);
    name.replace("8", "I", Qt::CaseInsensitive);
    name.replace("9", "O", Qt::CaseInsensitive);

    name = 'c' + name;

    if (!m_colors.contains(name)) {
        if (ts) {
            (*ts) << "\\definecolor{" << name << "}{rgb}{"
                  << color.redF() << ", " << color.greenF() << ", " << color.blueF() << "}\n";
        }
        m_colors[name] = true;
    }

    return name;
}

void QTikzPicture::begin(const QString& options)
{
    if (!ts) return;

    if (options.isEmpty()) {
        (*ts) << "\\begin{tikzpicture}\n";
    } else {
        (*ts) << "\\begin{tikzpicture}[" << options << "]\n";
    }
}

void QTikzPicture::end()
{
    if (!ts) return;

    (*ts) << "\\end{tikzpicture}\n";
}

void QTikzPicture::beginScope(const QString& options)
{
    if (!ts) return;

    if (options.isEmpty()) {
        (*ts) << "\\begin{scope}\n";
    } else {
        (*ts) << "\\begin{scope}[" << options << "]\n";
    }
}

void QTikzPicture::endScope()
{
    if (!ts) return;

    (*ts) << "\\end{scope}\n";
}

void QTikzPicture::newline()
{
    if (!ts) return;

    (*ts) << "\n";
}

void QTikzPicture::comment(const QString& text)
{
    if (!ts) return;

    (*ts) << "% " << text << "\n";
}

void QTikzPicture::path(const QPainterPath& path, const QString& options)
{
    if (!ts || path.isEmpty()) return;

    int i = 0;
    for (i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                (*ts) << " -- cycle;\n";
            }
            (*ts) << "\\draw[" << options << "] (" << element.x << ", " << element.y << ")";
        } else if (element.type == QPainterPath::LineToElement) {
            (*ts) << " -- (" << element.x << ", " << element.y << ")";
        }
    }
    if (i > 0) {
        (*ts) << " -- cycle;\n";
    }
}

void QTikzPicture::path(const QRectF& rect, const QString& options)
{
    if (!ts || rect.isEmpty()) return;

    (*ts) << "\\path";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " (" << rect.left() << ", " << rect.top()
          << ") rectangle (" << rect.right() << ", " << rect.bottom() << ");\n";
}

void QTikzPicture::clip(const QPainterPath& path)
{
    if (!ts || path.isEmpty()) return;

    (*ts) << "\\clip ";

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& element = path.elementAt(i);

        if (element.type == QPainterPath::MoveToElement) {
            if (i > 0) {
                (*ts) << " -- cycle";
                (*ts) << "\n      ";
            }
        } else if (element.type == QPainterPath::LineToElement) {
            (*ts) << " -- ";
        } else {
            qWarning() << "QTikzPicture::clip: uknown QPainterPath segment type";
        }
        (*ts) << "(" << element.x << ", " << element.y << ")";
    }

    (*ts) << " -- cycle;\n";
}

void QTikzPicture::clip(const QRectF& rect)
{
    if (!ts || rect.isEmpty()) return;

    (*ts) << "\\clip (" << rect.left() << ", " << rect.top()
          << ") rectangle (" << rect.right() << ", " << rect.bottom() << ");";
}

void QTikzPicture::circle(const QPointF& center, qreal radius, const QString& options)
{
    if (!ts || radius <= 0) return;

    (*ts) << "\\draw";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " (" << center.x() << ", " << center.y() << ") circle (" << radius << "cm);\n";
}

void QTikzPicture::line(const QPointF& p, const QPointF& q, const QString& options)
{
    if (!ts) return;

    (*ts) << "\\draw";
    if (!options.isEmpty()) {
        (*ts) << "[" << options << "]";
    }
    (*ts) << " (" << p.x() << ", " << p.y() << ") -- (" << q.x() << ", " << q.y() << ");\n";
}





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
