#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <QTimer>
#include <QPalette>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Button;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    QMap<quint32, Button*> buttons;
    QTimer runTimer;
    bool running;
public:
    MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow() override;

public slots:
    void start();
    void stop();

protected:
    virtual void keyPressEvent(QKeyEvent* ke) override;
    virtual void keyReleaseEvent(QKeyEvent* ev) override;

private:
    Ui::MainWindow *ui;
};

class Button : public QLabel
{
    Q_OBJECT

public:
    enum States { Normal, Pressed, Failed, Assign, Waiting};
    Button (quint32 key, const QString& caption, QWidget* parent = nullptr);
    States getState() const { return state;}

public slots:
    void setPressed();
    void setReleased();
    void setAssigning();
    void setWaiting();
    void setFailed();

    void onPress();

signals:
    void newKeyBind(quint32);

protected:
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void keyPressEvent(QKeyEvent *ev) override;

private:
    quint32 key;
    QMap<States, QPalette> palettes;
    States state;

    QElapsedTimer reactionTimer;
    QTimer failTimer;
    QTimer recoveryTimer;

    int misscount {0};
    int notpresscount {0};
    QVector<quint32> reactionTime;
};

#endif // MAINWINDOW_H
