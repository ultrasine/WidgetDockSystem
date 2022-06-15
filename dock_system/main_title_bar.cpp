#include "main_title_bar.hpp"
#include <QApplication>
#include <QToolButton>
#include <QMainWindow>
#include <QMouseEvent>
#include <QEvent>
#include <QStyle>
#include <QPoint>
#include <QLabel>
#include <QIcon>

struct title_bar_t::title_bar_private_t {
	bool mouse_left_pressing_ = false;
	QPoint move_start_point_;
	QMainWindow* window_ = nullptr;
	QLabel* icon_label_ = nullptr;
	QLabel* title_label_ = nullptr;
    click_widget_t* toolBar_seat_ = nullptr;
	QToolButton* button_mini_ = nullptr;
	QToolButton* button_max_ = nullptr;
	QToolButton* button_close_ = nullptr;
};
title_bar_t::title_bar_t(QWidget *parent) : QToolBar(parent) 
{
    title_bar_private_ = std::make_unique<title_bar_private_t>();
    title_bar_private_->window_ = (QMainWindow *)window();

    title_bar_private_->icon_label_ = new QLabel(this);
    title_bar_private_->title_label_ = new QLabel(this);
    title_bar_private_->icon_label_->setObjectName("title_icon_");
    title_bar_private_->icon_label_->setFixedSize(QSize(16,16));
    title_bar_private_->icon_label_->setScaledContents(true);
    title_bar_private_->icon_label_->setPixmap(title_bar_private_->window_->windowIcon().
        pixmap(QSize(16, 16)));
    title_bar_private_->icon_label_->setAlignment(Qt::AlignVCenter);
    title_bar_private_->icon_label_->setFixedWidth(title_bar_private_->icon_label_->width() + 10);

    title_bar_private_->title_label_->setObjectName("title_label_");
    title_bar_private_->title_label_->setText(parent->windowTitle());
    
    addWidget(title_bar_private_->icon_label_);
    addWidget(title_bar_private_->title_label_);

    title_bar_private_->toolBar_seat_ = new click_widget_t;
    title_bar_private_->toolBar_seat_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addWidget(title_bar_private_->toolBar_seat_);
  
    //title_bar_private_->button_mini_ = new QToolButton(this);
    title_bar_private_->button_max_ = new QToolButton(this);
    title_bar_private_->button_close_ = new QToolButton(this);
    title_bar_private_->button_max_->setFixedSize(48, 24);
    title_bar_private_->button_close_->setFixedSize(48, 24);
    //title_bar_private_->button_mini_->setIcon(QIcon(":/images/minimized.png"));
    title_bar_private_->button_max_->setIcon(QIcon(":/images/maximized.png"));
    title_bar_private_->button_close_->setIcon(QIcon(":/images/close.png"));
    
    //addWidget(title_bar_private_->button_mini_);
    addWidget(title_bar_private_->button_max_);
    addWidget(title_bar_private_->button_close_);

   // connect(title_bar_private_->button_mini_, SIGNAL(clicked(bool)),
            //title_bar_private_->window_, SLOT(showMinimized()));
    connect(title_bar_private_->button_max_, SIGNAL(clicked(bool)),
            this, SLOT(MaximizeButtonClicked()));
    connect(title_bar_private_->button_close_, SIGNAL(clicked(bool)),
            title_bar_private_->window_, SLOT(close()));
    connect(title_bar_private_->toolBar_seat_, &click_widget_t::clicked, [parent]() { 
        parent->showMaximized(); 
        });
}
void title_bar_t::set_window_tilte(const QString name) {
    title_bar_private_->title_label_->setText(name);
}


void title_bar_t::mouseDoubleClickEvent(QMouseEvent* /*event*/) {
    MaximizeButtonClicked();
}

void title_bar_t::MaximizeButtonClicked() {
    auto w = window();
    if (w->isMaximized())
        w->showNormal();
    else {
        w->showMaximized();
    }
}

void title_bar_t::updata_icon() {
    auto w = window();
    if (w->isMaximized()) {
        title_bar_private_->button_max_->setIcon(QIcon(":/images/restore.png"));
    }
    else {
        title_bar_private_->button_max_->setIcon(QIcon(":/images/maximized.png"));
    }
}
