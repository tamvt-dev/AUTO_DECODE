#ifndef NOTIFICATION_WIDGET_H
#define NOTIFICATION_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>

class NotificationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    enum NotificationType {
        Success,
        Error,
        Info,
        Warning
    };

    explicit NotificationWidget(const QString &message, NotificationType type, int timeout = 3000, QWidget *parent = nullptr);
    ~NotificationWidget() = default;

    void setTargetY(int y);
    void show();
    void animateOut();

signals:
    void finished();

private slots:
    void onAnimationFinished();
    void autoClose();

private:
    void setupUi();
    void applyStyles();
    QString getIconForType(NotificationType type) const;

    QString m_message;
    NotificationType m_type;
    int m_timeout;
    int m_targetY = 15;

    QLabel *messageLabel;
    QPropertyAnimation *slideInAnimation;
    QPropertyAnimation *slideOutAnimation;
};

#endif // NOTIFICATION_WIDGET_H
