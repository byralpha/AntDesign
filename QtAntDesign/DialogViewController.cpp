﻿#include "DialogViewController.h"

DialogViewController::DialogViewController(bool loginState, int parentW, int parentH, QWidget* parent)
	: QGraphicsView(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);  // 关键：启用透明背景
	scene = new QGraphicsScene(this);
	setScene(scene);
	setFrameStyle(QFrame::NoFrame);
	setStyleSheet("background: transparent; border: none;");
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// 关键性能优化
	setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	setCacheMode(QGraphicsView::CacheBackground);

	// 创建对话框
	//一旦你打算把一个 QWidget 放到场景中包装成 QGraphicsItem（比如通过 QGraphicsProxyWidget），那么这个 QWidget 就不能有父对象（parent），必须传 nullptr。
	//Qt 大坑
	dialog = new MaterialDialog(loginState, nullptr);
	contentDialogW = parentW * 0.40;
	contentDialogH = parentH * 0.41;
	setFixedSize(contentDialogW, contentDialogH);
	setSceneRect(QRectF(0, 0, contentDialogW, contentDialogH));

	// 设置图形代理
	proxy = scene->addWidget(dialog);
	proxy->setTransformOriginPoint(proxy->boundingRect().center());
	proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);

	// 动画
	// 缩放
	scaleAnim = new QPropertyAnimation(proxy, "scale");
	scaleAnim->setDuration(400);

	// 透明
	opacityAnim = new QPropertyAnimation(proxy, "opacity");
	opacityAnim->setDuration(400);

	// 曲线
	scaleAnim->setEasingCurve(QEasingCurve::OutCubic);
	opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

	// 动画组
	groupAnim = new QParallelAnimationGroup(this);
	groupAnim->addAnimation(scaleAnim);
	groupAnim->addAnimation(opacityAnim);

	connect(groupAnim, &QParallelAnimationGroup::finished, this, [this]()
		{
			if (m_isHide)
			{
				hide();
			}
		});

	// 深色遮罩
	mask = new MaskWidget(parentW, parentH, parent);
	connect(this, &DialogViewController::playMask, this, [this](bool isAddMask)
		{
			this->raise();
			if (isAddMask)
			{
				mask->opcaityAddAnim();
			}
			else
			{
				mask->opcaityReduceAnim();
			}
		});

	connect(mask, &MaskWidget::clicked, this, &DialogViewController::onMaskClicked);

	// 控制器转发信号
	connect(dialog, &MaterialDialog::successLogin, this, [this](bool loginState)
		{
			emit successLogin(loginState);
			hideAnim();
		});
	connect(dialog->standardDialog(), &StandardDialogPage::exitDialog, this, [this]()
		{
			hideAnim();
		});

	hide();
}

DialogViewController::~DialogViewController()
{
	if (dialog) dialog->deleteLater();
}

void DialogViewController::onMaskClicked()
{
	hideAnim();
}

void DialogViewController::showAnim(MaterialDialog::PageIndex index)
{
	updateDialogPositionAndSize(index);
	proxy->update();  // 强制刷新一次图像缓存 预热缓存，避免动画前卡顿
	show();
	dialog->showIndexPage(index);
	m_isHide = false;
	groupAnim->stop();
	scaleAnim->setStartValue(0.6);
	scaleAnim->setEndValue(1.0);
	opacityAnim->setStartValue(0.3);
	opacityAnim->setEndValue(1.0);
	groupAnim->start();
	emit playMask(true);
}

void DialogViewController::hideAnim()
{
	m_isHide = true;
	groupAnim->stop();
	scaleAnim->setStartValue(1.0);
	scaleAnim->setEndValue(0.6);
	opacityAnim->setStartValue(1.0);
	opacityAnim->setEndValue(0.3);
	groupAnim->start();
	emit playMask(false);
}

void DialogViewController::updateDialogPositionAndSize(MaterialDialog::PageIndex index)
{
	if (scene && proxy && mask && dialog)
	{
		int parentW = mainWindowSize.width(), parentH = mainWindowSize.height(), dialogW = 0, dialogH = 0;

		if (index == MaterialDialog::Login)
		{
			dialogW = contentDialogW;
			dialogH = contentDialogH;
		}
		else if (index == MaterialDialog::Profile)
		{
			dialogW = contentDialogW;
			dialogH = contentDialogH;
		}
		else if (index == MaterialDialog::Standard)
		{
			dialogW = standardDialogW;
			dialogH = standardDialogH;
		}

		//  1. 正确设置 stackedWidget 的尺寸
		setGeometry((parentW - dialogW) / 2, (parentH - dialogH) / 2, dialogW, dialogH);
		setFixedSize(dialogW, dialogH);

		//  2. 设置 scene 和 view
		scene->setSceneRect(0, 0, dialogW, dialogH);
		dialog->setFixedSize(dialogW, dialogH);
		proxy->resize(dialogW, dialogH);

		//  3. 更新 proxy 的位置和缩放中心（等 proxy 正确后再设）  proxy->resize 保证proxy->boundingRect()上有数据
		QRectF proxyRect = proxy->boundingRect();
		proxy->setTransformOriginPoint(proxyRect.center());

		//  4. 遮罩同步
		mask->resize(parentW, parentH);
		mask->move(0, 0);
	}
}

void DialogViewController::buildStandardDialog(int parentW, int parentH, int dialogW, int dialogH, QString title, QString text)
{
	if (scene && proxy && mask && dialog)
	{
		mainWindowSize = QSize(parentW, parentH);
		standardDialogW = dialogW;
		standardDialogH = dialogH;
		emit dialog->setStandardDialogText(title, text);
		showAnim(MaterialDialog::PageIndex::Standard);
	}
}

void DialogViewController::getParentSize(int parentW, int parentH)
{
	mainWindowSize = QSize(parentW, parentH);
}