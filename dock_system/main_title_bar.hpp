
#include <QToolBar>
#include <memory>

class title_bar_t : public QToolBar {
    Q_OBJECT
public:
    explicit title_bar_t(QWidget* parent = nullptr);
    void set_window_tilte(const QString);
    void updata_icon();
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
private slots:
    void MaximizeButtonClicked();
private:
    struct title_bar_private_t;
    std::unique_ptr<title_bar_private_t> title_bar_private_;
};

class click_widget_t : public QWidget
{
	Q_OBJECT
public:
	explicit click_widget_t(QWidget* parent = nullptr) : QWidget(parent) {}

signals:
	void clicked();

protected:
	void mouseDoubleClickEvent(QMouseEvent* ) override {
		emit  clicked();
	}
};