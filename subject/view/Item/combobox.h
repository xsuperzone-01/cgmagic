#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>

class ComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit ComboBox(QWidget *parent = nullptr);

    void showPopup();
    void hidePopup();
    void clearData();
    static void setPadding(QComboBox *cb);

signals:
    void getLists();

protected:
    void wheelEvent(QWheelEvent *e);
};

#endif // COMBOBOX_H
