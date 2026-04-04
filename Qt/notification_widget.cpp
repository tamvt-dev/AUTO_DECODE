#include "notification_widget.h"
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QPropertyAnimation>

NotificationWidget::NotificationWidget(const QString &message, NotificationType type, int timeout, QWidget *parent)
    : QWidget(parent)
    , m_message(message)
    , m_type(type)
    , m_timeout(timeout)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(350);
    setMinimumHeight(60);

    setupUi();
    applyStyles();
}

void NotificationWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);

    messageLabel = new QLabel(m_message);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet("color: white; font-size: 12px;");
    layout->addWidget(messageLabel);

    // Slide in animation (from right)
    slideInAnimation = new QPropertyAnimation(this, "geometry");
    slideInAnimation->setDuration(300);
    slideInAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Slide out animation (to right)
    slideOutAnimation = new QPropertyAnimation(this, "geometry");
    slideOutAnimation->setDuration(300);
    slideOutAnimation->setEasingCurve(QEasingCurve::InCubic);

    connect(slideOutAnimation, &QPropertyAnimation::finished, this, &NotificationWidget::onAnimationFinished);

    // Auto close timer
    QTimer::singleShot(m_timeout, this, &NotificationWidget::autoClose);
}

void NotificationWidget::applyStyles()
{
    QColor bgColor = getColorForType(m_type);
    QString styleSheet = QString(
        "QWidget { "
        "background-color: %1; "
        "border-radius: 8px; "
        "border: 1px solid %2;"
        "} "
    ).arg(bgColor.name()).arg(bgColor.darker(120).name());

    setStyleSheet(styleSheet);
}

QColor NotificationWidget::getColorForType(NotificationType type) const
{
    switch (type) {
        case Success: return QColor(46, 204, 113);    // Green #2ecc71
        case Error:   return QColor(231, 76, 60);     // Red #e74c3c
        case Warning: return QColor(243, 156, 18);    // Yellow #f39c12
        case Info:
        default:      return QColor(52, 152, 219);    // Blue #3498db
    }
}

QString NotificationWidget::getIconForType(NotificationType type) const
{
    switch (type) {
        case Success: return "✓";
        case Error:   return "✕";
        case Warning: return "⚠";
        case Info:    return "ℹ";
        default:      return "ℹ";
    }
}

void NotificationWidget::setTargetY(int y)
{
    m_targetY = y;
}

void NotificationWidget::show()
{
    // Position widget off-screen to the right
    if (parentWidget()) {
        int startX = parentWidget()->width() + 30;
        setGeometry(startX, m_targetY, width(), height());
    }

    QWidget::show();

    // Animate slide in from right
    if (parentWidget()) {
        int endX = parentWidget()->width() - width() - 15;

        slideInAnimation->setStartValue(geometry());
        slideInAnimation->setEndValue(QRect(endX, m_targetY, width(), height()));
        slideInAnimation->start();
    }
}

void NotificationWidget::animateOut()
{
    // Slide out to the right
    if (parentWidget()) {
        int endX = parentWidget()->width() + 30;
        int y = geometry().y();

        slideOutAnimation->setStartValue(geometry());
        slideOutAnimation->setEndValue(QRect(endX, y, width(), height()));
        slideOutAnimation->start();
    }
}

void NotificationWidget::autoClose()
{
    animateOut();
}

void NotificationWidget::onAnimationFinished()
{
    hide();
    emit finished();
}
