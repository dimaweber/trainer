#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#include <QFont>
#include <QKeyEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , running(false)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    quint32 scancodes [] =
    {
        49, 10, 11, 12, 13,
        23, 24, 25, 26, 27,
        66, 38, 39, 40, 41,
        50, 52, 53, 54, 55
    };

    for (int row=0; row<4; row++)
        for (int col=0; col<5; col++)
        {
            int i = row*5+col;
            quint32 id = scancodes[i];
            QString caption = QString("%1").arg(i+1);
            Button* ptr = new Button(id, caption, this);
            ui->gridLayout->addWidget(ptr, row, col);
            buttons[id] = ptr;

            connect (ptr, &Button::newKeyBind, [ptr, this, id](quint32 b)
            {
                if (buttons.contains(b))
                {
                    QMessageBox::information(this, "Can't bind", "Can't use this binding - already assigned to another button");
                }
                else
                {
                    buttons.remove(id);
                    buttons[b] = ptr;
                }
            });
        }

    connect(&runTimer, &QTimer::timeout, [this]()
    {
        Button* ptr = nullptr;
        do
        {
            int i = qrand() % buttons.count();
            quint32 id = buttons.keys()[i];
            ptr=  buttons[id];
        } while (ptr->getState() != Button::Normal);
        ptr->setWaiting();
    });

    connect (ui->startButton, &QPushButton::clicked, this, [this]()
    {
        if (running)
        {
            stop();
            ui->startButton->setText("Start");
        }
        else
        {
            start();
            ui->startButton->setText("Stop");
        }
        running = !running;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::start()
{
    runTimer.start(600);
}

void MainWindow::stop()
{
    runTimer.stop();
    for(Button* button: buttons)
        button->setReleased();
}

void MainWindow::keyPressEvent(QKeyEvent *ke)
{
    qDebug() << ke->text() << " press " << "mod: " << ke->modifiers() << " scan: " << ke->nativeScanCode() << " virt: " << ke->nativeVirtualKey();
    quint32 id = ke->nativeScanCode();
    if (buttons.contains(id))
    {
        buttons[id]->onPress();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *ev)
{
    qDebug() << ev->text() << " release";
    quint32 id = ev->nativeScanCode();
    if (buttons.contains(id))
    {
        buttons[id]->setReleased();
    }
}


Button::Button(quint32 key, const QString& caption, QWidget* parent)
    :QLabel(caption, parent), key(key), state(Normal)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(45, 45);
    setMaximumSize(60, 60);
    setAlignment(Qt::AlignCenter);

    QFont f = font();
    f.setBold(true);
    setFont(f);

    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(backgroundRole(), Qt::white);
    palettes[Normal] = pal;

    pal.setColor(backgroundRole(), Qt::green);
    palettes[Pressed] = pal;

    pal.setColor(backgroundRole(), Qt::red);
    palettes[Failed] = pal;

    pal.setColor(backgroundRole(), Qt::yellow);
    palettes[Assign] = pal;

    pal.setColor(backgroundRole(), Qt::blue);
    palettes[Waiting] = pal;

    failTimer.setSingleShot(true);
    recoveryTimer.setSingleShot(true);

    connect (&failTimer, &QTimer::timeout, [this]()
        {
           notpresscount++;
           setFailed();
        });

    connect (&recoveryTimer, &QTimer::timeout, [this]()
    {
        setReleased();
    });

    setReleased();
}

void Button::mousePressEvent(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonPress && ev->button() == Qt::RightButton)
    {
        qDebug() << text() << " r-click";
        setAssigning();
    }
}

void Button::keyPressEvent(QKeyEvent *ev)
{
    if (state != Assign)
    {
        QLabel::keyPressEvent(ev);
        return;
    }
    if (ev->key() == Qt::Key_Escape)
    {
        QLabel::keyPressEvent(ev);
        setReleased();
        return;
    }

    emit newKeyBind(ev->nativeScanCode());
    releaseKeyboard();
    setReleased();
}

void Button::setPressed()
{
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setPalette(palettes[Pressed]);
    state = Pressed;
}

void Button::setReleased()
{
    setFrameStyle(QFrame::Panel | QFrame::Raised);
    setPalette(palettes[Normal]);
    state = Normal;
    failTimer.stop();
    recoveryTimer.stop();
}

void Button::setAssigning()
{
    setFrameStyle(QFrame::WinPanel | QFrame::Sunken);
    setPalette(palettes[Assign]);
    grabKeyboard();
    state = Assign;
}

void Button::setWaiting()
{
    setFrameStyle(QFrame::Panel | QFrame::Raised);
    setPalette(palettes[Waiting]);
    state = Waiting;
    reactionTimer.start();
    failTimer.start(1000);
}

void Button::setFailed()
{
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setPalette(palettes[Failed]);
    state = Failed;
    recoveryTimer.start(200);
}

void Button::onPress()
{
    if (state == Waiting)
    {
        setPressed();
        failTimer.stop();
        quint32 elaps = reactionTimer.elapsed();
        reactionTime.append(elaps);
        qDebug() << "reaction: " <<  elaps << " ms";
    }
    else
    {
        setFailed();
        misscount++;
    }
}
