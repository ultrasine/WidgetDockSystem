/*******************************************************************************
** Qt Advanced Docking System
** Copyright (C) 2017 Uwe Kindler
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


//============================================================================
/// \file   DockAreaTitleBar.cpp
/// \author Uwe Kindler
/// \date   12.10.2018
/// \brief  Implementation of CDockAreaTitleBar class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "DockAreaTitleBar.h"

#include <QPushButton>
#include <QToolButton>
#include <QBoxLayout>
#include <QStyle>
#include <QMenu>
#include <QScrollArea>
#include <QMouseEvent>
#include <QDebug>
#include <QPointer>
#include <QIcon>
#include <map>

#include "DockAreaTitleBar_p.h"
#include "ads_globals.h"
#include "FloatingDockContainer.h"
#include "FloatingDragPreview.h"
#include "DockAreaWidget.h"
#include "DockOverlay.h"
#include "DockManager.h"
#include "DockWidget.h"
#include "DockWidgetTab.h"
#include "DockAreaTabBar.h"
#include "IconProvider.h"
#include "DockComponentsFactory.h"

#include <iostream>

namespace ads
{

/**
 * Private data class of CDockAreaTitleBar class (pimpl)
 */
struct DockAreaTitleBarPrivate
{
	CDockAreaTitleBar* _this;
	QPointer<CTitleBarButton> TabsMenuButton;

	//add more buttons for   play/pause  setting   resetting  capture
	QPointer<CTitleBarStateButton> PlayPauseButton;
	QPointer<CTitleBarStateButton> SetButton;
	QPointer<tTitleBarButton> DefaultSetButton;
	QPointer<tTitleBarButton> CaptureButton;
	QPointer<tTitleBarButton> UndockButton;
	QPointer<tTitleBarButton> CloseButton;
	
	std::map<CDockManager::eConfigFlag, tTitleBarButton*> Config2Button;

	QBoxLayout* Layout;
	CDockAreaWidget* DockArea;
	CDockAreaTabBar* TabBar;
	bool MenuOutdated = true;
	QMenu* TabsMenu;
	QList<tTitleBarButton*> DockWidgetActionsButtons;

	QPoint DragStartMousePos;
	eDragState DragState = DraggingInactive;
	IFloatingWidget* FloatingWidget = nullptr;


	/**
	 * Private data constructor
	 */
	DockAreaTitleBarPrivate(CDockAreaTitleBar* _public);

	/**
	 * Creates the title bar close and menu buttons
	 */
	void createButtons();

	/*
	* Set the button visability
	*/
	void setButtonVisable(CDockManager::eConfigFlag witch, bool show);

	/**
	 * Creates the internal TabBar
	 */
	void createTabBar();

	/**
	 * Convenience function for DockManager access
	 */
	CDockManager* dockManager() const
	{
		return DockArea->dockManager();
	}

	/**
	 * Returns true if the given config flag is set
	 * Convenience function to ease config flag testing
	 */
	static bool testConfigFlag(CDockManager::eConfigFlag Flag)
	{
		return CDockManager::testConfigFlag(Flag);
	}

	/**
	 * Test function for current drag state
	 */
	bool isDraggingState(eDragState dragState) const
	{
		return this->DragState == dragState;
	}

	tTitleBarButton* getButton(CDockManager::eConfigFlag witch);

	/**
	 * Starts floating
	 */
	void startFloating(const QPoint& Offset);

	/**
	 * Makes the dock area floating
	 */
	IFloatingWidget* makeAreaFloating(const QPoint& Offset, eDragState DragState);
};// struct DockAreaTitleBarPrivate

//============================================================================
DockAreaTitleBarPrivate::DockAreaTitleBarPrivate(CDockAreaTitleBar* _public) :
	_this(_public)
{

}


//============================================================================
void DockAreaTitleBarPrivate::createButtons()
{
	QSizePolicy ButtonSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	// Tabs menu button
	TabsMenuButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasTabsMenuButton));
	TabsMenuButton->setObjectName("tabsMenuButton");
	TabsMenuButton->setAutoRaise(true);
	TabsMenuButton->force_visible(false);
	TabsMenuButton->setDisabled(true);
	TabsMenuButton->hide();
	TabsMenuButton->setPopupMode(QToolButton::InstantPopup);
	internal::setButtonIcon(TabsMenuButton, QStyle::SP_TitleBarUnshadeButton, ads::DockAreaMenuIcon);
	QMenu* TabsMenu = new QMenu(TabsMenuButton);
#ifndef QT_NO_TOOLTIP
	TabsMenu->setToolTipsVisible(true);
#endif
	_this->connect(TabsMenu, SIGNAL(aboutToShow()), SLOT(onTabsMenuAboutToShow()));
	_this->connect(_this->tabBar(), &CDockAreaTabBar::tabInserted, [this](int) {
		if (_this->tabBar()->count() > 5) {
			TabsMenuButton->force_visible(true);
			TabsMenuButton->setEnabled(true);
			TabsMenuButton->show();
		}
			
		});
	_this->connect(_this->tabBar(), &CDockAreaTabBar::removingTab, [this](int) {
		if (_this->tabBar()->count() < 5) {
			TabsMenuButton->force_visible(false);
			TabsMenuButton->setDisabled(true);
			TabsMenuButton->hide();
		}
		});

	TabsMenuButton->setMenu(TabsMenu);
	internal::setToolTip(TabsMenuButton, u8"所有窗口");
	TabsMenuButton->setSizePolicy(ButtonSizePolicy);
	Layout->addWidget(TabsMenuButton, 0);
	_this->connect(TabsMenuButton->menu(), SIGNAL(triggered(QAction*)),
		SLOT(onTabsMenuActionTriggered(QAction*)));

	Config2Button.clear();

	QIcon posIcon = QIcon(":/images/setting32.png");
	QIcon negIcon = QIcon(":/images/setting32.png");

	//SetButton
	if (testConfigFlag(CDockManager::DockAreaHasSettingButton))
	{
		SetButton = new CTitleBarStateButton(posIcon, negIcon, u8"打开设置", u8"关闭设置");
		SetButton->setCheckable(true);
		SetButton->setObjectName("SetButton");
		SetButton->setAutoRaise(true);
		internal::setToolTip(SetButton, u8"设置");
		SetButton->setSizePolicy(ButtonSizePolicy);
		SetButton->setIconSize(QSize(16, 16));
		Layout->addWidget(SetButton, 0);
		SetButton->show();
		Config2Button.insert({ CDockManager::DockAreaHasSettingButton, SetButton.data() });
	}

	//PlayPauseButton
	if (testConfigFlag(CDockManager::DockAreaHasPlayPauseButton))
	{
		posIcon = QIcon(":/images/pause64.png");
		negIcon = QIcon(":/images/player64.png");
		PlayPauseButton = new CTitleBarStateButton(posIcon, negIcon, u8"暂停", u8"播放");
		PlayPauseButton->setObjectName("PlayPauseButton");
		PlayPauseButton->setAutoRaise(true);
		PlayPauseButton->setSizePolicy(ButtonSizePolicy);
		PlayPauseButton->setIconSize(QSize(18, 18));
		Layout->addWidget(PlayPauseButton, 0);
		PlayPauseButton->show();
		Config2Button.insert({ CDockManager::DockAreaHasPlayPauseButton, dynamic_cast<CTitleBarButton*>(PlayPauseButton.data()) });
	}

	//DefaultSetButton
	if (testConfigFlag(CDockManager::DockAreaHasDefaultSetButton))
	{
		DefaultSetButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasDefaultSetButton));
		DefaultSetButton->setObjectName("DefaultSetButton");
		DefaultSetButton->setAutoRaise(true);
		internal::setToolTip(DefaultSetButton, u8"恢复");
		DefaultSetButton->setIcon(QIcon(":/images/reset64.png"));
		DefaultSetButton->setSizePolicy(ButtonSizePolicy);
		DefaultSetButton->setIconSize(QSize(18, 18));
		Layout->addWidget(DefaultSetButton, 0);
		DefaultSetButton->show();
		Config2Button.insert({ CDockManager::DockAreaHasDefaultSetButton, DefaultSetButton.data() });
	}


	//capture button
	if (testConfigFlag(CDockManager::DockAreaHasCaptureButton))
	{
		CaptureButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasCaptureButton));
		CaptureButton->setObjectName("captureButton");
		CaptureButton->setAutoRaise(true);
		internal::setToolTip(CaptureButton, u8"截图");
		CaptureButton->setIcon(QIcon(":/images/graghshot64.png"));
		CaptureButton->setSizePolicy(ButtonSizePolicy);
		CaptureButton->setIconSize(QSize(18, 18));
		Layout->addWidget(CaptureButton, 0);
		CaptureButton->show();
		Config2Button.insert({ CDockManager::DockAreaHasCaptureButton, CaptureButton.data() });
	}

	// Undock button
	UndockButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasUndockButton));
	UndockButton->setObjectName("detachGroupButton");
	UndockButton->setAutoRaise(true);
	internal::setToolTip(UndockButton, u8"浮动");
	UndockButton->setIcon(QIcon(":/images/load32.png"));
	UndockButton->setSizePolicy(ButtonSizePolicy);
	UndockButton->setIconSize(QSize(16, 16));
	Layout->addWidget(UndockButton, 0);
	_this->connect(UndockButton, SIGNAL(clicked()), SLOT(onUndockButtonClicked()));
	Config2Button.insert({ CDockManager::DockAreaHasUndockButton, UndockButton.data() });

	// Close button
	CloseButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasCloseButton));
	CloseButton->setObjectName("dockAreaCloseButton");
	CloseButton->setAutoRaise(true);
	internal::setButtonIcon(CloseButton, QStyle::SP_TitleBarCloseButton, ads::DockAreaCloseIcon);
	if (testConfigFlag(CDockManager::DockAreaCloseButtonClosesTab))
	{
		internal::setToolTip(CloseButton,u8"关闭激活窗口");
	}
	else
	{
		internal::setToolTip(CloseButton, u8"关闭窗口");
	}
	CloseButton->setSizePolicy(ButtonSizePolicy);
	CloseButton->setIconSize(QSize(16, 16));
	Layout->addWidget(CloseButton, 0);
	_this->connect(CloseButton, SIGNAL(clicked()), SLOT(onCloseButtonClicked()));
	Config2Button.insert({ CDockManager::DockAreaHasCloseButton, CloseButton.data()});
}


//============================================================================
tTitleBarButton* DockAreaTitleBarPrivate::getButton(CDockManager::eConfigFlag witch)
{
	auto iter = Config2Button.find(witch);
	return iter == Config2Button.end()? nullptr : iter->second;
}


//============================================================================
void DockAreaTitleBarPrivate::setButtonVisable(CDockManager::eConfigFlag witch, bool show)
{
	auto iter = Config2Button.find(witch);
	if (iter != Config2Button.end())
	{
		CDockManager::setConfigFlag(witch,  show);
		iter->second->setVisible(show);
	}
}


//============================================================================
void DockAreaTitleBarPrivate::createTabBar()
{
	TabBar = componentsFactory()->createDockAreaTabBar(DockArea);
    TabBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
	Layout->addWidget(TabBar);
	_this->connect(TabBar, SIGNAL(tabClosed(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(tabOpened(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(tabInserted(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(removingTab(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(tabMoved(int, int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(currentChanged(int)), SLOT(onCurrentTabChanged(int)));
	_this->connect(TabBar, SIGNAL(tabBarClicked(int)), SIGNAL(tabBarClicked(int)));
	_this->connect(TabBar, SIGNAL(elidedChanged(bool)), SLOT(markTabsMenuOutdated()));
}


//============================================================================
IFloatingWidget* DockAreaTitleBarPrivate::makeAreaFloating(const QPoint& Offset, eDragState DragState)
{
	QSize Size = DockArea->size();
	this->DragState = DragState;
	bool OpaqueUndocking = CDockManager::testConfigFlag(CDockManager::OpaqueUndocking) ||
		(DraggingFloatingWidget != DragState);
	CFloatingDockContainer* FloatingDockContainer = nullptr;
	IFloatingWidget* FloatingWidget;
	if (OpaqueUndocking)
	{
		FloatingWidget = FloatingDockContainer = new CFloatingDockContainer(DockArea);
	}
	else
	{
		auto w = new CFloatingDragPreview(DockArea);
		QObject::connect(w, &CFloatingDragPreview::draggingCanceled, [=]()
		{
			this->DragState = DraggingInactive;
		});
		FloatingWidget = w;
	}

    FloatingWidget->startFloating(Offset, Size, DragState, nullptr);
    if (FloatingDockContainer)
    {
		auto TopLevelDockWidget = FloatingDockContainer->topLevelDockWidget();
		if (TopLevelDockWidget)
		{
			TopLevelDockWidget->emitTopLevelChanged(true);
		}
    }

	//本区域的所有dockwidgit
	auto count = DockArea->dockWidgetsCount();
	for (int i = 0; i < count; i++)
	{
		DockArea->dockWidget(i)->runSettingHandler(false);
		DockArea->dockWidget(i)->runDockStateHandler(false);
	}

	//清空当前titlebar上设置按钮的状态
	SetButton->resetState();
	SetButton->setChecked(false);

	return FloatingWidget;
}


//============================================================================
void DockAreaTitleBarPrivate::startFloating(const QPoint& Offset)
{
	FloatingWidget = makeAreaFloating(Offset, DraggingFloatingWidget);
}


//============================================================================
CDockAreaTitleBar::CDockAreaTitleBar(CDockAreaWidget* parent) :
	QFrame(parent),
	d(new DockAreaTitleBarPrivate(this))
{
	d->DockArea = parent;

	setObjectName("dockAreaTitleBar");
	d->Layout = new QBoxLayout(QBoxLayout::LeftToRight);
	d->Layout->setContentsMargins(0, 0, 0, 0);
	d->Layout->setSpacing(0);
	setLayout(d->Layout);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	d->createTabBar();
	d->Layout->addWidget(new CSpacerWidget(this));
	d->createButtons();

	set_button_handlers();

    setFocusPolicy(Qt::NoFocus);
}


//============================================================================
CDockAreaTitleBar::~CDockAreaTitleBar()
{
	if (!d->CloseButton.isNull())
	{
		delete d->CloseButton;
	}

	if (!d->TabsMenuButton.isNull())
	{
		delete d->TabsMenuButton;
	}

	if (!d->UndockButton.isNull())
	{
		delete d->UndockButton;
	}

	if (!d->CaptureButton.isNull())
	{
		delete d->CaptureButton;
	}

	if (!d->DefaultSetButton.isNull())
	{
		delete d->DefaultSetButton;
	}

	if (!d->SetButton.isNull())
	{
		delete d->SetButton;
	}

	delete d;
}


//============================================================================
CDockAreaTabBar* CDockAreaTitleBar::tabBar() const
{
	return d->TabBar;
}

//============================================================================
void CDockAreaTitleBar::markTabsMenuOutdated()
{
	if(DockAreaTitleBarPrivate::testConfigFlag(CDockManager::DockAreaDynamicTabsMenuButtonVisibility))
	{
		bool hasElidedTabTitle = false;
		for (int i = 0; i < d->TabBar->count(); ++i)
		{
			if (!d->TabBar->isTabOpen(i))
			{
				continue;
			}
			CDockWidgetTab* Tab = d->TabBar->tab(i);
			if(Tab->isTitleElided())
			{
				hasElidedTabTitle = true;
				break;
			}
		}
		bool visible = (hasElidedTabTitle && (d->TabBar->count() > 1));
		QMetaObject::invokeMethod(d->TabsMenuButton, "setVisible", Qt::QueuedConnection, Q_ARG(bool, visible));
	}
	d->MenuOutdated = true;
}

//============================================================================
void CDockAreaTitleBar::onTabsMenuAboutToShow()
{
	if (!d->MenuOutdated)
	{
		return;
	}

	QMenu* menu = d->TabsMenuButton->menu();
	menu->clear();
	for (int i = 0; i < d->TabBar->count(); ++i)
	{
		if (!d->TabBar->isTabOpen(i))
		{
			continue;
		}
		auto Tab = d->TabBar->tab(i);
		QAction* Action = menu->addAction(Tab->icon(), Tab->text());
		internal::setToolTip(Action, Tab->toolTip());
		Action->setData(i);
	}

	d->MenuOutdated = false;
}


//============================================================================
void CDockAreaTitleBar::onCloseButtonClicked()
{
    ADS_PRINT("CDockAreaTitleBar::onCloseButtonClicked");
	if (!d->DockArea->allow_close_area()) {
		d->DockArea->hide_close_btn();
		return;
	}

	if (d->testConfigFlag(CDockManager::DockAreaCloseButtonClosesTab))
	{
		d->TabBar->tab(d->TabBar->currentIndex())->dockWidget()->runCloseHandler();
		d->TabBar->closeTab(d->TabBar->currentIndex());
	}
	else
	{
		d->DockArea->closeArea();
	}

	if (d->TabBar->count() == 1)
		d->DockArea->runCloseHandler();
}


//============================================================================
void CDockAreaTitleBar::onUndockButtonClicked()
{
	if (d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		d->makeAreaFloating(mapFromGlobal(QCursor::pos()), DraggingInactive);
	}
}


//============================================================================
void CDockAreaTitleBar::onTabsMenuActionTriggered(QAction* Action)
{
	int Index = Action->data().toInt();
	d->TabBar->setCurrentIndex(Index);
	emit tabBarClicked(Index);
}


//============================================================================
void CDockAreaTitleBar::updateDockWidgetActionsButtons()
{
	CDockWidget* DockWidget = d->TabBar->currentTab()->dockWidget();
	if (!d->DockWidgetActionsButtons.isEmpty())
	{
		for (auto Button : d->DockWidgetActionsButtons)
		{
			d->Layout->removeWidget(Button);
			delete Button;
		}
		d->DockWidgetActionsButtons.clear();
	}

	auto Actions = DockWidget->titleBarActions();
	if (Actions.isEmpty())
	{
		return;
	}

	int InsertIndex = indexOf(d->TabsMenuButton);
	for (auto Action : Actions)
	{
		auto Button = new CTitleBarButton(true, this);
		Button->setDefaultAction(Action);
		Button->setAutoRaise(true);
		Button->setPopupMode(QToolButton::InstantPopup);
		Button->setObjectName(Action->objectName());
		d->Layout->insertWidget(InsertIndex++, Button, 0);
		d->DockWidgetActionsButtons.append(Button);
	}
}


//============================================================================
void CDockAreaTitleBar::onCurrentTabChanged(int Index)
{
	if (Index < 0)
	{
		return;
	}

	if (d->testConfigFlag(CDockManager::DockAreaCloseButtonClosesTab))
	{
		CDockWidget* DockWidget = d->TabBar->tab(Index)->dockWidget();
		d->CloseButton->setEnabled(DockWidget->features().testFlag(CDockWidget::DockWidgetClosable));

		//便利所有的tab并调用通知回调
		int count = d->TabBar->count();
		for (int i = 0; i < count; i++)
		{
			if (i != Index)
			{
				d->TabBar->tab(i)->dockWidget()->runTabChangeHandler(false);

				//设置按钮状态清空
				d->SetButton.data()->resetState();
				d->SetButton.data()->setChecked(false);
			}
		}

		d->TabBar->tab(Index)->dockWidget()->runTabChangeHandler(true);

		//这里只恢复播放暂停按钮的状态
		if (!d->PlayPauseButton.isNull())
			d->PlayPauseButton.data()->setState(d->TabBar->tab(Index)->dockWidget()->playState());
	}

	updateDockWidgetActionsButtons();
}


//============================================================================
QAbstractButton* CDockAreaTitleBar::button(TitleBarButton which) const
{
	switch (which)
	{
	case TitleBarButtonTabsMenu: return d->TabsMenuButton;
	case TitleBarButtonUndock: return d->UndockButton;
	case TitleBarButtonClose: return d->CloseButton;
	default:
		return nullptr;
	}
}


//============================================================================
void CDockAreaTitleBar::setVisible(bool Visible)
{
	Super::setVisible(Visible);
	markTabsMenuOutdated();

	if(!Visible)
		d->DragState = DraggingInactive;
}


//============================================================================
void CDockAreaTitleBar::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		ev->accept();
		d->DragStartMousePos = ev->pos();
		d->DragState = DraggingMousePressed;

		if (CDockManager::testConfigFlag(CDockManager::FocusHighlighting))
		{
			d->TabBar->currentTab()->setFocus(Qt::OtherFocusReason);
		}
		return;
	}
	Super::mousePressEvent(ev);
}


//============================================================================
void CDockAreaTitleBar::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
        ADS_PRINT("CDockAreaTitleBar::mouseReleaseEvent");
		ev->accept();
		auto CurrentDragState = d->DragState;
		d->DragStartMousePos = QPoint();
		d->DragState = DraggingInactive;
		if (DraggingFloatingWidget == CurrentDragState)
		{
			d->FloatingWidget->finishDragging();
		}

		return;
	}
	Super::mouseReleaseEvent(ev);
}


//============================================================================
void CDockAreaTitleBar::mouseMoveEvent(QMouseEvent* ev)
{
	if (!d->DockArea->allow_move())
		return;

	Super::mouseMoveEvent(ev);
	if (allow_move_handler_ && !allow_move_handler_())
	{
		return;
	}

	if (!(ev->buttons() & Qt::LeftButton) || d->isDraggingState(DraggingInactive))
	{
		d->DragState = DraggingInactive;
		return;
	}


    // move floating window
    if (d->isDraggingState(DraggingFloatingWidget))
    {
        d->FloatingWidget->moveFloating();
        return;
    }

	// If this is the last dock area in a floating dock container it does not make
	// sense to move it to a new floating widget and leave this one
	// empty
	if (d->DockArea->dockContainer()->isFloating()
	 && d->DockArea->dockContainer()->visibleDockAreaCount() == 1)
	{
		return;
	}

	int DragDistance = (d->DragStartMousePos - ev->pos()).manhattanLength();
	if (DragDistance >= CDockManager::startDragDistance())
	{
        ADS_PRINT("CDockAreaTitlBar::startFloating");
		d->startFloating(d->DragStartMousePos);
		auto Overlay = d->DockArea->dockManager()->containerOverlay();
		Overlay->setAllowedAreas(OuterDockAreas);
	}

	return;
}


//============================================================================
void CDockAreaTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (allow_move_handler_ && !allow_move_handler_())
	{
		return;
	}
	// If this is the last dock area in a dock container it does not make
	// sense to move it to a new floating widget and leave this one
	// empty
	if (d->DockArea->dockContainer()->isFloating() && d->DockArea->dockContainer()->dockAreaCount() == 1)
	{
		return;
	}

	if (!d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		return;
	}
	d->makeAreaFloating(event->pos(), DraggingInactive);
}


//============================================================================
void CDockAreaTitleBar::contextMenuEvent(QContextMenuEvent* ev)
{
	ev->accept();
	if (d->isDraggingState(DraggingFloatingWidget))
	{
		return;
	}

	/*QMenu Menu(this);
	auto Action = Menu.addAction(u8"分离区域", this, SLOT(onUndockButtonClicked()));
	Action->setEnabled(d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable));
	Menu.addSeparator();
	Action = Menu.addAction(u8"关闭当前区域", this, SLOT(onCloseButtonClicked()));
	Action->setEnabled(d->DockArea->features().testFlag(CDockWidget::DockWidgetClosable));
	Menu.addAction(u8"关闭其他区域", d->DockArea, SLOT(closeOtherAreas()));
	Menu.exec(ev->globalPos());*/
}


//============================================================================
void CDockAreaTitleBar::insertWidget(int index, QWidget *widget)
{
	d->Layout->insertWidget(index, widget);
}


//============================================================================
int CDockAreaTitleBar::indexOf(QWidget *widget) const
{
	return d->Layout->indexOf(widget);
}

//============================================================================
void CDockAreaTitleBar::set_allow_move(const std::function<bool()>& handler) {
	allow_move_handler_ = handler;
}

//============================================================================
bool CDockAreaTitleBar::allow_move() {
	if (allow_move_handler_ == nullptr) return true;
	return allow_move_handler_();
}


//============================================================================
void CDockAreaTitleBar::set_button_visible(CDockManager::eConfigFlag witch, bool show)
{
	tTitleBarButton* button = d->getButton(witch);
	if (button != nullptr)
	{
		button->setVisible(show);
	}
}


//============================================================================
void CDockAreaTitleBar::set_button_tip(CDockManager::eConfigFlag witch, QString tip)
{
	tTitleBarButton* button = d->getButton(witch);
	if (button != nullptr)
	{
		button->setToolTip(tip);
	}
}


//============================================================================
void CDockAreaTitleBar::set_state_button_icon(CDockManager::eConfigFlag witch, QIcon& positive_icon, QIcon& negative_icon)
{
	tTitleBarButton* button = d->getButton(witch);
	CTitleBarStateButton* tbsb = dynamic_cast<CTitleBarStateButton*>(button);
	if (button != nullptr && tbsb != nullptr)
	{
		tbsb->setIcons(positive_icon, negative_icon);
	}
}


//============================================================================
void CDockAreaTitleBar::reset_state(CDockManager::eConfigFlag witch)
{
	tTitleBarButton* button = d->getButton(witch);
	CTitleBarStateButton* tbsb = dynamic_cast<CTitleBarStateButton*>(button);
	if (button != nullptr && tbsb != nullptr)
	{
		tbsb->resetState();
	}
}

void CDockAreaTitleBar::set_button_checked(CDockManager::eConfigFlag witch, bool is_checked)
{
	tTitleBarButton* button = d->getButton(witch);
	button->setChecked(is_checked);
	CTitleBarStateButton* tbsb = dynamic_cast<CTitleBarStateButton*>(button);
	if (button != nullptr && tbsb != nullptr)
	{
		tbsb->setState(!is_checked);
	}
}

//============================================================================
void CDockAreaTitleBar::set_state_button_tips(CDockManager::eConfigFlag witch, QString posTip, QString negTip)
{
	tTitleBarButton* button = d->getButton(witch);
	CTitleBarStateButton* tbsb = dynamic_cast<CTitleBarStateButton*>(button);
	if (button != nullptr && tbsb != nullptr)
	{
		tbsb->setTips(posTip, negTip);
	}
}


//============================================================================
void CDockAreaTitleBar::set_button_handlers()
{
	//播放暂停按钮回调
	connect(d->PlayPauseButton.data(), &CTitleBarStateButton::clicked, [this]() {
		int index = d->TabBar->currentIndex();
		if (index >= 0)
		{
			CDockWidget* DockWidget = d->TabBar->tab(index)->dockWidget();
			DockWidget->runPlayPauseHandler(d->PlayPauseButton->state());
		}
	});

	//设置按钮回调
	connect(d->SetButton.data(), &CTitleBarStateButton::clicked, [this]() {
		int index = d->TabBar->currentIndex();
		if (index >= 0)
		{
			CDockWidget* DockWidget = d->TabBar->tab(index)->dockWidget();
			DockWidget->runSettingHandler(d->SetButton->state());
		}
	});

	//恢复按钮
	connect(d->DefaultSetButton.data(), &CTitleBarStateButton::clicked, [this]() {
		int index = d->TabBar->currentIndex();
		if (index >= 0)
		{
			CDockWidget* DockWidget = d->TabBar->tab(index)->dockWidget();
			DockWidget->runResettingHandler();
		}
	});

	//截图按钮
	connect(d->CaptureButton.data(), &CTitleBarStateButton::clicked, [this]() {
		int index = d->TabBar->currentIndex();
		if (index >= 0)
		{
			CDockWidget* DockWidget = d->TabBar->tab(index)->dockWidget();
			DockWidget->runCaptureHandler();
		}
	});
}


//============================================================================
void CDockAreaTitleBar::set_button_icon(CDockManager::eConfigFlag witch, QIcon& icon)
{
	tTitleBarButton* button = d->getButton(witch);
	if (button != nullptr)
	{
		button->setIcon(icon);
	}
}


//============================================================================
CTitleBarButton::CTitleBarButton(bool visible, QWidget* parent)
	: tTitleBarButton(parent),
	  Visible(visible),
	  HideWhenDisabled(CDockManager::testConfigFlag(CDockManager::DockAreaHideDisabledButtons))
{
    setFocusPolicy(Qt::NoFocus);
}

//============================================================================
bool CTitleBarButton::event(QEvent *ev)
{
	if (QEvent::EnabledChange == ev->type() && HideWhenDisabled)
	{
		// force setVisible() call 
		// Calling setVisible() directly here doesn't work well when button is expected to be shown first time
		QMetaObject::invokeMethod(this, "setVisible", Qt::QueuedConnection, Q_ARG(bool, isEnabled()));
	}

	return Super::event(ev);
}

//============================================================================
void CTitleBarButton::setVisible(bool visible)
{
	// 'visible' can stay 'true' if and only if this button is configured to generaly visible:
	visible = visible && this->Visible;

	// 'visible' can stay 'true' unless: this button is configured to be invisible when it is disabled and it is currently disabled:
	if (visible && HideWhenDisabled)
	{
		visible = isEnabled();
	}

	Super::setVisible(visible);
}

void CTitleBarButton::force_visible(bool v) {
	Super::setVisible(v);
}


//============================================================================

CTitleBarStateButton::CTitleBarStateButton(QIcon& posIcon, QIcon& negicon , QString posTip, QString negTip, bool visible, QWidget* parent)
	:CTitleBarButton(visible, parent), State(true),
	PositiveIcon(posIcon), NegativeIcon(negicon),
	PositiveTip(posTip), NegativeTip(negTip)
{
	//最开始设置为positive
	setState(true);
}

bool CTitleBarStateButton::state()
{
	return State;
}

void CTitleBarStateButton::setState(bool s)
{
	State = s;
	if (State)
	{
		setToolTip(PositiveTip);
		setIcon(PositiveIcon);
	}
	else
	{
		setToolTip(NegativeTip);
		setIcon(NegativeIcon);
	}

	emit clickState(State);
}


void CTitleBarStateButton::resetState()
{
	setState(true);
}


void CTitleBarStateButton::setIcons(QIcon& posIcon, QIcon& negIcon)
{
	PositiveIcon = posIcon;
	NegativeIcon = negIcon;
	setState(State);
}

void CTitleBarStateButton::setTips(QString posTip, QString negTip)
{
	PositiveTip = posTip;
	NegativeTip = negTip;
	if (State)
		setToolTip(PositiveTip);
	else
		setToolTip(NegativeTip);
}

bool CTitleBarStateButton::event(QEvent* ev)
{
	if (QEvent::MouseButtonRelease == ev->type())
	{
		setState(!State);
	}

	return CTitleBarButton::event(ev);
}
//============================================================================

//============================================================================
CSpacerWidget::CSpacerWidget(QWidget* Parent /*= 0*/) : Super(Parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setStyleSheet("border: none; background: none;");
}

} // namespace ads

//---------------------------------------------------------------------------
// EOF DockAreaTitleBar.cpp
