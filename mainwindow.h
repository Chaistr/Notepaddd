#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QNetworkAccessManager>  // 网络请求管理
#include <QNetworkRequest>        // 网络请求
#include <QNetworkReply>          // 网络响应
#include <QJsonObject>            // JSON对象
#include <QJsonArray>             // JSON数组
#include <QJsonDocument>          // JSON文档处理
#include <QSslConfiguration>
#include <QCoreApplication>
#include <QSslSocket>// SSL配置（Schannel相关）
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>
#include <QLabel>
#include <QList>
#include <QTextEdit>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QColor>
#include <QColorDialog>
#include <QMessageBox>
#include <QFontDialog>
#include <QTextOption>
#include <QDesktopServices>
#include <QUrl>
#include <QTextCursor>
#include <string>
#include <QActionGroup>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
//
#include <qinputdialog.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
//自己加的函数
public:
    void Init();
    QNetworkAccessManager *networkManager;
    QString apiKey;

private:
    //自己加的菜单变量
    QMenu* file;
    QMenu* edit;
    QMenu* format;
    QMenu* view;
    QMenu* help;
    //给每个menu添加action
    QAction* file_new;
    QAction* file_open;
    QAction* file_save;
    QAction* file_saveas;//另列为
    QAction* file_print;//打印
    QAction* file_exit;//退出
    //编辑的acition
    QAction* edit_undo;
    QAction* edit_cut;
    QAction* edit_copy;
    QAction* edit_past;
    QAction* edit_del;
    QAction* edit_bing;
    QAction* edit_find;
    QAction* edit_find_n;
    QAction* edit_replace;
    QAction* edit_goto;
    QAction* edit_all;
    QAction* edit_date;
    //格式
    QAction *form_autoline;
    QAction *form_font;
    QAction *form_color;

    //查看
    QMenu* menuChild;
    QAction *zoomin;//放大
    QAction *zoomout;//缩小
    //帮助
    QAction *help_github;
    QAction *help_about;

    //建立toolBar
    QToolBar* tbar;
    QJsonArray chatHistory;



    //statusbar
    QStatusBar* stbar;//状态栏
    QLabel* lb_fileinfo;
    QLabel* lb_time;


    QTabWidget* TW;


    //定义QList容器
    QList<QTextEdit*> Lst;  //list里面都是textedit

    //textedit的id和name
    int ID;
    QString filename;//

    //状态栏显示时间
    QDateTime myTime;//时间
    QTimer* timer;


private slots:
    void Flush_time();//刷新时间
    void New_Edit();//新建文件
    void File_Open();//打开文件
    void Del_Tab(int index);//删除Tab
    void Click(int index);
    void File_Save();
    void Unreal_Fun();

    void Set_Color();
    void Set_Font();

    void Zoom();
    void callDeepSeekAPI();
    void onNetworkReplyFinished(QNetworkReply *reply);
QTextEdit* getCurrentEditor();
    void GoGithub();
    void About();
    //new by chaistr
    void Bing_Search();
    void Find_Text();
    void Find_Next();
    void Replace_Text();
    void Goto_Line();
    void Toggle_Auto_Wrap();

private:
    //new by cha
    QString lastFindText;






};
#endif // MAINWINDOW_H
