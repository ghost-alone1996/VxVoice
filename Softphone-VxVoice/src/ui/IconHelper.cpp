#include "IconHelper.h"
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

QIcon IconHelper::createStatusDot(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(QColor(0, 0, 0, 40));
    p.setPen(Qt::NoPen);
    p.drawEllipse(1, 2, size - 2, size - 2);

    p.setBrush(color);
    p.drawEllipse(1, 1, size - 2, size - 2);

    p.setBrush(QColor(255, 255, 255, 120));
    p.drawEllipse(size / 4, size / 4, size / 3, size / 3);

    return QIcon(pix);
}

QIcon IconHelper::createUserIcon(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);

    int headRadius = size / 4;
    int headCenterX = size / 2;
    int headCenterY = size / 3;
    p.drawEllipse(QPoint(headCenterX, headCenterY), headRadius, headRadius);

    QPainterPath path;
    int bodyTop = size / 2 + 2;
    path.moveTo(size * 0.15, size - 2);
    path.cubicTo(size * 0.2, bodyTop, size * 0.8, bodyTop, size * 0.85, size - 2);
    path.lineTo(size * 0.15, size - 2);
    p.drawPath(path);

    return QIcon(pix);
}

QIcon IconHelper::createSettingsIcon(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 2.5));
    p.setBrush(Qt::NoBrush);

    int center = size / 2;
    int r = size / 3;
    p.drawEllipse(QPoint(center, center), r, r);

    p.setPen(QPen(color, 2.2, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 6; ++i) {
        p.save();
        p.translate(center, center);
        p.rotate(i * 60);
        p.drawLine(0, -r - 1, 0, -r - 4);
        p.restore();
    }

    return QIcon(pix);
}

QIcon IconHelper::createBellIcon(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);

    QPainterPath path;
    path.moveTo(size * 0.5, size * 0.15);
    path.cubicTo(size * 0.7, size * 0.2, size * 0.8, size * 0.55, size * 0.88, size * 0.75);
    path.lineTo(size * 0.12, size * 0.75);
    path.cubicTo(size * 0.2, size * 0.55, size * 0.3, size * 0.2, size * 0.5, size * 0.15);
    p.drawPath(path);

    p.drawEllipse(QPoint(size / 2, size * 0.85), size / 8, size / 8);

    return QIcon(pix);
}

QIcon IconHelper::createPhoneIcon(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(color);

    // Desenha auricular de telefone ergonômico
    QPainterPath path;
    path.moveTo(size * 0.25, size * 0.40);
    path.cubicTo(size * 0.25, size * 0.28, size * 0.38, size * 0.25, size * 0.48, size * 0.32);
    path.lineTo(size * 0.52, size * 0.42);
    path.cubicTo(size * 0.45, size * 0.48, size * 0.48, size * 0.55, size * 0.58, size * 0.65);
    path.cubicTo(size * 0.68, size * 0.75, size * 0.75, size * 0.78, size * 0.81, size * 0.71);
    path.lineTo(size * 0.91, size * 0.75);
    path.cubicTo(size * 0.98, size * 0.85, size * 0.95, size * 0.98, size * 0.83, size * 0.98);
    path.cubicTo(size * 0.52, size * 0.98, size * 0.25, size * 0.71, size * 0.25, size * 0.40);
    path.closeSubpath();
    p.drawPath(path);

    return QIcon(pix);
}

QIcon IconHelper::createCloseIcon(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 3.2, Qt::SolidLine, Qt::RoundCap));

    int pad = size * 0.28;
    p.drawLine(pad, pad, size - pad, size - pad);
    p.drawLine(size - pad, pad, pad, size - pad);

    return QIcon(pix);
}

QIcon IconHelper::createBackspaceIcon(const QColor &color, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Contorno elegante do botão backspace (formato de etiqueta com ponta à esquerda)
    QPainterPath outline;
    qreal leftTip = size * 0.12;
    qreal shoulderX = size * 0.38;
    qreal topY = size * 0.22;
    qreal bottomY = size * 0.78;
    qreal rightX = size * 0.90;
    qreal radius = size * 0.08;

    outline.moveTo(leftTip, size * 0.50);
    outline.lineTo(shoulderX, topY);
    outline.lineTo(rightX - radius, topY);
    outline.quadTo(rightX, topY, rightX, topY + radius);
    outline.lineTo(rightX, bottomY - radius);
    outline.quadTo(rightX, bottomY, rightX - radius, bottomY);
    outline.lineTo(shoulderX, bottomY);
    outline.closeSubpath();

    // Borda do símbolo
    p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(outline);

    // Cruz interna ('X' de delete)
    qreal x1 = size * 0.52;
    qreal x2 = size * 0.74;
    qreal y1 = size * 0.38;
    qreal y2 = size * 0.62;

    p.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    p.drawLine(QPointF(x2, y1), QPointF(x1, y2));

    return QIcon(pix);
}

QPixmap IconHelper::createUserAvatarPixmap(const QColor &bgColor, const QColor &fgColor, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Círculo de fundo
    p.setBrush(bgColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(1, 1, size - 2, size - 2);

    // Borda sutil
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(fgColor.lighter(130), 2.0));
    p.drawEllipse(2, 2, size - 4, size - 4);

    // Cabeça do avatar
    p.setBrush(fgColor);
    p.setPen(Qt::NoPen);
    int headRadius = size / 5;
    int headCenterX = size / 2;
    int headCenterY = size / 3 + 2;
    p.drawEllipse(QPoint(headCenterX, headCenterY), headRadius, headRadius);

    // Ombros / Corpo do avatar
    QPainterPath path;
    int bodyTop = size / 2 + size / 10;
    path.moveTo(size * 0.22, size * 0.78);
    path.cubicTo(size * 0.26, bodyTop, size * 0.74, bodyTop, size * 0.78, size * 0.78);
    path.lineTo(size * 0.22, size * 0.78);
    p.drawPath(path);

    return pix;
}
