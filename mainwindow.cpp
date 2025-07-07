#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>  // 确保包含JSON文档头文件
#include <QJsonObject>    // 确保包含JSON对象头文件
#include <QJsonArray>     // 确保包含JSON数组头文件
#include <QNetworkReply>
#include <QSslError>
 #include <QNetworkAccessManager>
#include <QTabWidget>
#include <QStatusBar>
#include<QSsl>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReplyFinished);
    apiKey = "  ";//输入deepseek密钥
    //设置窗口属性
    this->setMinimumSize(680, 480);
    this->setWindowIcon(QIcon(":/icon/icon.png"));
    this->setWindowTitle("记事本");
    chatHistory = QJsonArray();


    Init();//c初始化主窗口

    // 初始化自动换行菜单项状态（默认为启用自动换行）
    form_autoline->setChecked(true);
    this->New_Edit();//启动后先调用

    // 原: 可能包含OpenSSL特定配置
    // 现: 使用Qt原生实现

    //连接文件菜单的几个aciton
    connect(this->file_new, &QAction::triggered, this, &MainWindow::New_Edit);
    connect(file_open, &QAction::triggered, this, &MainWindow::File_Open);
    connect(TW, &QTabWidget::tabCloseRequested, this, &MainWindow::Del_Tab);
    connect(TW, &QTabWidget::tabBarClicked, this, &MainWindow::Click);//定位点击位置
    connect(file_save, &QAction::triggered, this, &MainWindow::File_Save);
    connect(file_saveas, &QAction::triggered, this, &MainWindow::File_Save);
    connect(file_print, &QAction::triggered, this, &MainWindow::Unreal_Fun);
    connect(file_exit, &QAction::triggered, this, &MainWindow::close);//关闭，close是自带的槽函数
    // Bing 搜索框
    connect(edit_bing, &QAction::triggered, this, &MainWindow::Bing_Search);
    connect(edit_find, &QAction::triggered, this, &MainWindow::Find_Text);
    connect(edit_replace, &QAction::triggered, this, &MainWindow::Replace_Text);
    connect(edit_goto, &QAction::triggered, this, &MainWindow::Goto_Line);
    connect(edit_find_n, &QAction::triggered, this, &MainWindow::Find_Next);
    // 自动换行
    connect(form_autoline, &QAction::triggered, this, &MainWindow::Toggle_Auto_Wrap);


    //编辑连接
        //全选
    connect(this->edit_all, &QAction::triggered, this->Lst.at(this->TW->currentIndex()), [this]() {
        this->Lst.at(this->TW->currentIndex())->selectAll();
    });
        //undo撤销
    connect(edit_undo, &QAction::triggered, this->Lst.at(this->TW->currentIndex()),[this](){
        this->Lst.at(this->TW->currentIndex())->undo();
    });
        //剪切
    connect(edit_cut, &QAction::triggered, this->Lst.at(this->TW->currentIndex()),[this](){
        this->Lst.at(this->TW->currentIndex())->cut();
    });
        //复制
    connect(edit_copy, &QAction::triggered, this->Lst.at(this->TW->currentIndex()),[this](){
        this->Lst.at(this->TW->currentIndex())->copy();
    });
    //监听文字是否被选中
    connect(Lst.at(this->TW->currentIndex()), &QTextEdit::selectionChanged,[=](){
        //如果选中信号发送，将action重新设置为可点击
        edit_copy->setEnabled(!Lst.at(this->TW->currentIndex())->textCursor().selectedText().isEmpty());
        edit_cut->setEnabled(!Lst.at(this->TW->currentIndex())->textCursor().selectedText().isEmpty());
        edit_del->setEnabled(!Lst.at(this->TW->currentIndex())->textCursor().selectedText().isEmpty());
    });

        //粘贴
    connect(edit_past, &QAction::triggered, this->Lst.at(this->TW->currentIndex()),[this](){
        this->Lst.at(this->TW->currentIndex())->paste();
    });
        //删除
    connect(edit_del, &QAction::triggered, this->Lst.at(this->TW->currentIndex()),[this](){
        //textcursor返回光标位置和选中范围，然后用removSelectedText删除
        this->Lst.at(this->TW->currentIndex())->textCursor().removeSelectedText();
    });


    //格式
    connect(form_color,&QAction::triggered, this, &MainWindow::Set_Color);
    connect(form_font, &QAction::triggered, this, &MainWindow::Set_Font);
    //查看
    connect(zoomin, &QAction::triggered, this, &MainWindow::Zoom);//放大
    connect(zoomout, &QAction::triggered,[=](){
        QFont font = Lst.at(TW->currentIndex())->font();
        int fontSize = font.pointSize();
        if (fontSize > 10) {
            font.setPointSize(fontSize - 2);
            Lst.at(TW->currentIndex())->setFont(font);
        }
    });//缩小
    //帮助
    connect(help_github, &QAction::triggered, this, &MainWindow::GoGithub);
    connect(help_about,&QAction::triggered, this, &MainWindow::About);

    //刷新时间
    connect(this->timer, &QTimer::timeout, this, &MainWindow::Flush_time);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete networkManager;
    delete TW;
}
// mainwindow.cpp
void MainWindow::callDeepSeekAPI()
{
    // 获取当前文本编辑框
    QTextEdit *currentEditor = getCurrentEditor();
    if (!currentEditor) return;

    // 获取用户输入
    QString userInput = currentEditor->toPlainText();
    if (userInput.isEmpty()) return;

    // DeepSeek API配置
    const QUrl apiUrl("https://api.deepseek.com/v1/chat/completions");

    // 创建请求
    QNetworkRequest request(apiUrl);
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_3); // 使用最新TLS版本

    request.setSslConfiguration(sslConfig);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    // 构建请求JSON
    QJsonObject requestBody;
    requestBody["model"] = "deepseek-chat";

    // 设置对话消息（保留历史）
    QJsonArray messages;

    // 添加历史消息（如果有）
    for (const auto& msg : chatHistory) {
        messages.append(msg);
    }

    // 添加当前用户消息
    QJsonObject userMessage;
    QJsonObject responseFormat;
    responseFormat["type"] = "json_object";
    userMessage["response_format"] = responseFormat;
    userMessage["role"] = "user";
    userMessage["content"] = userInput;
    messages.append(userMessage);

    requestBody["messages"] = messages;
    requestBody["temperature"] = 1.3;
    requestBody["max_tokens"] = 1000;

    // 保存到历史
    chatHistory.append(userMessage);

    // 发送POST请求
    QJsonDocument doc(requestBody);
    QByteArray data = doc.toJson();
    QNetworkReply *reply = networkManager->post(request, data);
    statusBar()->showMessage("正在调用DeepSeek API...");

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onNetworkReplyFinished(reply);
    });


    connect(networkManager, &QNetworkAccessManager::sslErrors, [](QNetworkReply *reply, const QList<QSslError> &errors) {
        for (const QSslError &error : errors) {
            qDebug() << "SSL error:" << error.errorString();
        }

    });
    // 显示加载状态

}
// mainwindow.cpp
void MainWindow::onNetworkReplyFinished(QNetworkReply *reply)
{
    statusBar()->clearMessage();

    // 确保reply有效
    if (!reply) {
        statusBar()->showMessage("Error: Invalid network reply");
        return;
    }

    // 检查网络请求是否成功
    if (reply->error() == QNetworkReply::NoError) {
        // 读取服务器响应
        QByteArray responseData = reply->readAll();

        // 调试输出：打印完整的响应数据
        qDebug() << "API Response:" << QString::fromUtf8(responseData);

        // 解析JSON数据
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

        // 验证JSON有效性
        if (!responseDoc.isNull() && responseDoc.isObject()) {
            QJsonObject responseObj = responseDoc.object();

            // 处理API错误
            if (responseObj.contains("error")) {
                QJsonObject errorObj = responseObj["error"].toObject();
                QString errorType = errorObj["type"].toString();
                QString errorMessage = errorObj["message"].toString();

                // 显示详细的API错误信息
                QMessageBox::critical(this, "API Error",
                                      QString("Type: %1\nMessage: %2").arg(errorType, errorMessage));

                statusBar()->showMessage("API request failed");
                return;
            }

            // 处理正常响应
            if (responseObj.contains("choices") && responseObj["choices"].isArray()) {
                QJsonArray choices = responseObj["choices"].toArray();

                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices[0].toObject();

                    if (firstChoice.contains("message") && firstChoice["message"].isObject()) {
                        QJsonObject message = firstChoice["message"].toObject();

                        if (message.contains("content") && message["content"].isString()) {
                            QString content = message["content"].toString();

                            // 验证内容非空
                            if (!content.isEmpty()) {
                                // 保存到聊天历史
                                QJsonObject assistantMessage;
                                assistantMessage["role"] = "assistant";
                                assistantMessage["content"] = content;
                                chatHistory.append(assistantMessage);

                                // 显示回复
                                QTextEdit *currentEditor = getCurrentEditor();
                                if (currentEditor) {
                                    currentEditor->append("\n\nDeepSeek AI:\n" + content);
                                    statusBar()->showMessage("Response received");
                                } else {
                                    QMessageBox::warning(this, "UI Error",
                                                         "Failed to get current editor. Please try again.");
                                    statusBar()->showMessage("Error: Cannot display response");
                                }
                            } else {
                                QMessageBox::information(this, "Empty Response",
                                                         "The AI returned an empty response.");
                                statusBar()->showMessage("Empty response from AI");
                            }
                        } else {
                            QMessageBox::critical(this, "Response Format Error",
                                                  "Invalid message format: 'content' missing or not a string.");
                        }
                    } else {
                        QMessageBox::critical(this, "Response Format Error",
                                              "Invalid message format: 'message' object missing.");
                    }
                } else {
                    QMessageBox::information(this, "No Response",
                                             "The API returned an empty 'choices' array.");
                    statusBar()->showMessage("No response from API");
                }
            } else {
                QMessageBox::critical(this, "Response Format Error",
                                      "Invalid response format: 'choices' array missing.");
            }
        } else {
            // 提供更详细的JSON解析错误信息
            //QMessageBox::critical(this, "JSON Parse Error",
                                  //QString("Failed to parse JSON response:\n%1").arg(QString::fromUtf8(responseData)));
            //statusBar()->showMessage("JSON parsing failed");
        }
    } else {
        // 处理各种网络错误
        QString errorDetails = QString("Network Error %1: %2")
                                   .arg(reply->error())
                                   .arg(reply->errorString());

        // 针对常见错误提供额外提示
        if (reply->error() == QNetworkReply::HostNotFoundError) {
            errorDetails += "\n\nCheck your internet connection and API endpoint URL.";
        } else if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errorDetails += "\n\nInvalid API key. Please check your credentials.";
        } else if (reply->error() == QNetworkReply::ContentNotFoundError) {
            errorDetails += "\n\nThe requested API endpoint was not found. Check your URL.";
        }

        QMessageBox::critical(this, "Network Request Failed", errorDetails);
        statusBar()->showMessage("Network error occurred");
    }

    // 释放网络资源
    reply->deleteLater();
}
// mainwindow.cpp
QTextEdit* MainWindow::getCurrentEditor()
{
    int index = TW->currentIndex();
    if (index >= 0) {
        QTextEdit* editor = qobject_cast<QTextEdit*>(TW->widget(index));
        if (!editor) {
            qDebug() << "Failed to cast widget to QTextEdit at index:" << index;
        }
        return editor;
    }
    qDebug() << "Invalid current index:" << index;
    return nullptr;
}
void MainWindow::Init()
{
    //添加menu
    file = menuBar()->addMenu("文件(&F)");
    edit = menuBar()->addMenu("编辑(&E)");
    format = menuBar()->addMenu("格式(&O)");
    view = menuBar()->addMenu("查看(&V)");
    help = menuBar()->addMenu("帮助(&H)");
    //添加action
    file_new = new QAction(QIcon(":/icon/icon1.png"),"新建(&N)         Ctrl+N");//创建action
    file->addAction(file_new);//添加到file菜单
    file_open = new QAction(QIcon(":/icon/icon2.png"),"打开(&O)         Ctrl+O");
    file->addAction(file_open);
    file_save = new QAction(QIcon(":/icon/icon3.png"),"保存(&S)         Ctrl+S");
    this->file_save->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));//设置快捷键
    file->addAction(file_save);
    file_saveas = new QAction(QIcon(":/icon/icon4.png"),"另列为(&A)         Ctrl+A");
    file->addAction(file_saveas);

    file_print = new QAction(QIcon(":/icon/icon6.png"),"打印(&P)         Ctrl+P");
    file->addAction(file_print);

     file->addSeparator();//添加分割线
    file_exit = new QAction(QIcon(":/icon/icon7.png"),"退出(&X)");
    file->addAction(file_exit);

    //添加编辑的action
    edit_undo = new QAction(QIcon(":/icon/icon8.png"), "撤销(&U)         Ctrl+Z");
    edit->addAction(edit_undo);
     edit->addSeparator();//分割线
    edit_cut = new QAction(QIcon(":/icon/icon9.png"), "剪切(&Y)         Ctrl+X");
        edit_cut->setEnabled(false);//默认设置为不可剪切
    edit->addAction(edit_cut);

    edit_copy = new QAction(QIcon(":/icon/icon10.png"), "复制(&C)         Ctrl+C");
    edit_copy->setEnabled(false);//默认设置为不可复制
    edit->addAction(edit_copy);

    edit_past = new QAction(QIcon(":/icon/icon11.png"), "粘贴(&P)         Ctrl+V");
    edit->addAction(edit_past);
    edit_del = new QAction(QIcon(":/icon/icon12.png"), "删除(&L)         Del");
     edit_del->setEnabled(false);//默认设置为不可删除
    edit->addAction(edit_del);

    edit->addSeparator();//分割线
    edit_bing = new QAction(QIcon(":/icon/icon14.png"), "bing搜索(&E)       Ctrl+E");
    edit->addAction(edit_bing);
    edit_find = new QAction(QIcon(":/icon/icon13.png"), "查找(&F)         Ctrl+F");
    edit->addAction(edit_find);
    edit_find_n = new QAction(QIcon(":/icon/icon15.png"), "查找下一个(&N)         F3");
    edit->addAction(edit_find_n);
    edit_replace = new QAction(QIcon(":/icon/icon16.png"), "替换(&R)         Ctrl+H");
    edit->addAction(edit_replace);
    edit_goto = new QAction(QIcon(":/icon/icon17.png"), "转到(&G)         Ctrl+G");
    edit->addAction(edit_goto);
    edit->addSeparator();//分割线
    edit_all = new QAction(QIcon(":/icon/icon18.png"), "全选(&A)         Ctrl+A");
    edit->addAction(edit_all);
    /*
    edit_date = new QAction(QIcon(":/icon/icon19.png"), "时间/日期(&D)         F5");
    edit->addAction(edit_date);
*/
    //添加格式的action
    form_autoline = new QAction(QIcon(":/icon/icon20.png"), "自动换行(&W)");
    format->addAction(form_autoline);
    form_font = new QAction(QIcon(":/icon/icon21.png"), "字体(&F)");
    format->addAction(form_font);
    form_color = new QAction(QIcon(":/icon/icon22.png"), "颜色");
    format->addAction(form_color);
    //添加查看的action
    menuChild = new QMenu("缩放(&Z)");//添加子菜单
    zoomin = new QAction(QIcon(":/icon/icon23.png"), "放大  Ctrl");
    zoomin->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Plus));
    zoomout = new QAction(QIcon(":/icon/icon24.png"), "缩小");
    view->addMenu(menuChild);
    menuChild->addAction(zoomin);
    menuChild->addAction(zoomout);

   // view_status = new QAction(QIcon(":/icon/icon25.png"), "状态栏(S)");
   // view->addAction(view_status);

    //添加帮助的action
    help_github = new QAction(QIcon(":/icon/icon26.png"), "github仓库");
    help->addAction(help_github);
     help->addSeparator();//添加分割线
    help_about = new QAction(QIcon(":/icon/icon27.png"), "关于记事本");
    help->addAction(help_about);


    //建立toolBar
    tbar = addToolBar("文件");

    tbar->addAction(file_open);
    tbar->addAction(file_save);
    tbar->addSeparator();//添加分割线
    tbar->addAction(edit_undo);
    tbar->addAction(edit_cut);
    tbar->addAction(edit_copy);
    tbar->addAction(edit_past);
    tbar->addAction(edit_del);
    tbar->addAction(edit_find);
    tbar->addAction(edit_all);
    tbar->addSeparator();
    tbar->addAction(form_font);
    tbar->addAction(form_color);
    QAction *callDeepSeek = new QAction(QIcon(":/icon/icon28.png"), "调用 DeepSeek API");
    help->addAction(callDeepSeek);
    connect(callDeepSeek, &QAction::triggered, this, &MainWindow::callDeepSeekAPI);


    //tabwidget 标签页
    this->TW = new QTabWidget(this);
    this->setCentralWidget(TW);//设置标签页居中？

    //状态栏statusbar
    this->stbar = statusBar();

    this->lb_fileinfo = new QLabel("我是文件信息");
    stbar->addWidget(lb_fileinfo);


    //设置时间为右对齐
     lb_time = new QLabel("我是时间");

     // 向状态栏添加时间标签作为永久小部件，才能实现右对齐。
     stbar->addPermanentWidget(lb_time);


    //初始化textedit的变量
    this->ID = 0;
   this->filename = "无标题";


    //刷新状态栏时间
     this->Flush_time();
    this->timer = new QTimer(this);

    /*启动定时器，并设置定时器的时间间隔为 1000 毫秒（即 1 秒）。
     * 这意味着每隔 1 秒，
     * 定时器将触发一个信号（timeout 信号），
     * 可以连接该信号到相应的槽函数，
     * 实现定时执行特定的操作。
*/
    timer->start(1000);//时间启动器






}

void MainWindow::Flush_time()
{
    this->myTime = QDateTime::currentDateTime();//获取当前时间
    //设置显示格式
    QString str = this->myTime.toString("yyyy-MM-dd hh:mm:ss ddd");
    this->lb_time->setText(" 当前时间:" + str);
}

void MainWindow::New_Edit()
{
    QFont defaultFont("微软雅黑", 12);
    QTextEdit* te = new QTextEdit();//这里加不加this需要考虑
    te->setFont(defaultFont);
    this->Lst.append(te);//添加到list中

    this->TW->addTab(Lst.at(ID), filename);//将第id个textedit添加到标签页中
    this->lb_fileinfo->setText(filename);
    this->ID++;
    this->filename = "无标题";
    TW->setTabsClosable(1);//设置标签页为可关闭状态

}

void MainWindow::File_Open()
{
    qDebug() << "打开文件";
    QString Fname;
    // 获取打开文件的路径和名称
    Fname = QFileDialog::getOpenFileName(this, "打开文件", "./", "文本文件 (*.txt);;所有文件 (*.*)");
    if(Fname.isEmpty())
    {
        QMessageBox::information(this, "警告", "文件打开失败");
        return;
    }
    qDebug() << "打开文件成功";

    //读取文件
    QFile file(Fname);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::information(this, "错误", "读取文件失败");
    }

    QFileInfo fileinfo(Fname);
  this->filename = fileinfo.fileName();
    this->New_Edit();
    this->TW->setCurrentIndex(ID-1);//切换到当前打开的标签页

    //建立edit装内容
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);  // 设置文件的编码方式为UTF-8，以便读取文字不会乱码
    while(!in.atEnd())
    {
        QString line = in.readLine();//读取一行


        this->Lst.at(ID-1)->append(line);
    }
    file.close();
    qDebug() << "读取文件成功";


}

/*槽函数会被调用并且会自动接收到要关闭的标签页的索引作为参数。*/
void MainWindow::Del_Tab(int index)
{
    TW->removeTab(index);
}

void MainWindow::Click(int index)
{
    this->TW->setCurrentIndex(index);
    lb_fileinfo->setText("文件名:" + TW->tabText(index));
}

void MainWindow::File_Save()
{

    //进行写操作
    //Lst.at(index) 返回列表 Lst 中位于索引 index 处的元素
    //获取tab名字
    QString defaultFileName = this->TW->tabText(this->TW->currentIndex());

    QString sFname = QFileDialog::getSaveFileName(this, "保存文件", defaultFileName,"文本文件 (*.txt);;所有文件 (*.*)");
    if(sFname.isEmpty())
    {
        QMessageBox::information(this, "错误提示", "文件保存失败");
        return;
    }


    QFile file(sFname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
     {
         QMessageBox::information(this, "错误提示", "无法打开文件进行保存");
         return;
     }
    //进行写操作
    QString txt = this->Lst.at(this->TW->currentIndex())->toPlainText();
    file.write(txt.toUtf8());

    QString newFileName = QFileInfo(sFname).fileName();
    this->TW->setTabText(this->TW->currentIndex(), newFileName);
    file.close();


    QMessageBox::information(this, "成功提示", "文件保存成功");

}

void MainWindow::Set_Color()
{
    QColor mycolor = QColorDialog::getColor();
    this->Lst.at(TW->currentIndex())->setTextColor(mycolor);
    //判断颜色是有效的不是
    if(!mycolor.isValid())
    {
//        QMessageBox::information(this, "提示", "颜色获取失败");
        return;
    }

}

void MainWindow::Set_Font()
{
    bool ok;
    QFont myfont = QFontDialog::getFont(&ok);
    if(ok)
    {
        this->Lst.at(TW->currentIndex())->setFont(myfont);
    }
    else
    {
//        QMessageBox::information(this, "提示", "字体设置失败");
        return;
    }
}

void MainWindow::Zoom()
{
    this->Lst.at(TW->currentIndex())->setFont(QFont("微软雅黑", 20));
}

void MainWindow::GoGithub()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Chaistr/Notepaddd/"));
    return;
}

void MainWindow::About()
{
     QMessageBox::about(this, "关于记事本",
                        "软件名：window记事本\n"
                        "版本号:1.0版本\n"
                        "技术支持：Qt、CSDN个人博客\n"
                        "版权所有:Microsoft Windows\n"
                        "再次创作者：张敏，李铎，李宇阳");
}

void MainWindow::Unreal_Fun()
{
    QMessageBox::information(this, "温馨提示", "无效设置模式...");
    return;
}

//new by chaistr
void MainWindow::Bing_Search()
{
    QTextEdit* currentEdit = Lst.at(TW->currentIndex()); // 获取当前编辑器
    QTextCursor cursor = currentEdit->textCursor();

    QString searchContent;
    if(cursor.hasSelection()) // 如果用户已经选中文本，则直接搜索选中的文本
    {
        searchContent = cursor.selectedText();
    }
    else // 如果用户没有选中文本，则提示用户输入搜索内容
    {
        bool ok;
        searchContent = QInputDialog::getText(
            this,                     // 父窗口
            "Bing 搜索框搜",            // 对话框标题
            "输入搜索内容:",          // 提示信息
            QLineEdit::Normal,        // 输入模式
            "",                       // 默认值
            &ok                       // 用户是否点击了确定按钮
            );
        if(!ok || searchContent.isEmpty()) // 如果用户取消输入或输入为空，直接返回
        {
            return;
        }
    }


    // 构造 Bing 搜索功能搜 URL
    QString url = "https://www.bing.com/search?q=" + QUrl::toPercentEncoding(searchContent);

    // 使用 QDesktopServices 打开系统默认浏览器访问该 URL
    if (!QDesktopServices::openUrl(QUrl(url)))
    {
        QMessageBox::warning(this, "搜索错误", "无法打开浏览器进行搜索。");
    }
}
void MainWindow::Find_Text() {
    // 创建查找对话框
    bool ok;
    QString findText = QInputDialog::getText(
        this,
        "查找",
        "查找内容:",
        QLineEdit::Normal,
        "",
        &ok
        );

    if (ok && !findText.isEmpty()) {
        QTextEdit* currentEdit = Lst.at(TW->currentIndex()); // 获取当前编辑器

        // 使用 QTextEdit::find 方法查找文本，该方法会自动选中找到的文本
        bool found = currentEdit->find(findText);

        if (!found) {
            // 如果未找到，从文档开头开始查找
            QTextCursor cursor = currentEdit->textCursor();
            cursor.movePosition(QTextCursor::Start);
            currentEdit->setTextCursor(cursor);
            found = currentEdit->find(findText);

            if (!found) {
                // 如果仍然未找到，显示提示信息
                QMessageBox::information(this, "查找结果", "未找到匹配的文本。");
            }
        }

        // 保存查找文本，用于后续的查找下一个操作
        this->lastFindText = findText;
    }
}

void MainWindow::Find_Next() {
    if (!this->lastFindText.isEmpty()) {
        QTextEdit* currentEdit = Lst.at(TW->currentIndex()); // 获取当前编辑器

        bool found = currentEdit->find(lastFindText);

        if (!found) {
            // 如果未找到，从文档开头开始查找
            QTextCursor cursor = currentEdit->textCursor();
            cursor.movePosition(QTextCursor::Start);
            currentEdit->setTextCursor(cursor);
            found = currentEdit->find(lastFindText);

            if (!found) {
                // 如果仍然未找到，显示提示信息
                QMessageBox::information(this, "查找结果", "未找到更多匹配的文本。");
            }
        }
    } else {
        // 如果没有之前的查找文本，调用 Find_Text 函数
        Find_Text();
    }
}

void MainWindow::Replace_Text() {
    QTextEdit* currentEdit = Lst.at(TW->currentIndex()); // 获取当前编辑器
    QTextCursor cursor = currentEdit->textCursor();

    if (cursor.hasSelection()) { // 如果用户已经选中文本，则替换选中的部分
        QString selectedText = cursor.selectedText();
        bool ok;
        QString replaceText = QInputDialog::getText(
            this,
            "替换",
            "替换为:",
            QLineEdit::Normal,
            "",
            &ok
            );

        if (ok) {
            cursor.insertText(replaceText); // 替换选中的文本
            currentEdit->setTextCursor(cursor); // 更新光标位置
        }
    } else { // 如果用户没有选中文本，则替换所有匹配项
        // 创建替换对话框
        bool ok;
        QString findText = QInputDialog::getText(
            this,
            "替换",
            "查找内容:",
            QLineEdit::Normal,
            "",
            &ok
            );

        if (ok && !findText.isEmpty()) {
            QString replaceText = QInputDialog::getText(
                this,
                "替换",
                "替换为:",
                QLineEdit::Normal,
                "",
                &ok
                );

            if (ok) {
                QString text = currentEdit->toPlainText(); // 获取当前文本
                int count = text.count(findText); // 查找匹配项的总数

                if (count > 0) {
                    // 替换所有匹配项
                    text.replace(findText, replaceText);
                    currentEdit->setPlainText(text); // 设置新文本

                    // 显示替换结果
                    QMessageBox::information(this, "替换结果", QString("已替换 %1 处匹配项。").arg(count));
                } else {
                    // 如果没有找到匹配项
                    QMessageBox::information(this, "替换结果", "未找到匹配项。");
                }
            }
        }
    }
}

void MainWindow::Goto_Line() {
    // 创建转到对话框
    bool ok;
    int lineNum = QInputDialog::getInt(this, "转到", "行号:", 1, 1, 1000000, 1, &ok);

    if (ok) {
        QTextEdit* currentEdit = Lst.at(TW->currentIndex()); // 获取当前编辑器
        if (lineNum > 0 && lineNum <= currentEdit->document()->blockCount()) {
            QTextCursor cursor = currentEdit->textCursor();
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lineNum - 1);
            currentEdit->setTextCursor(cursor);
        } else {
            QMessageBox::warning(this, "警告", "行号超出范围！");
        }
    }
}

//
void MainWindow::Toggle_Auto_Wrap()
{
    QTextEdit* currentEdit = Lst.at(TW->currentIndex()); // 获取当前编辑器

    // 切换自动换行状态
    if(currentEdit->lineWrapMode() == QTextEdit::WidgetWidth)
    {
        currentEdit->setLineWrapMode(QTextEdit::NoWrap); // 关闭自动换行
    }
    else
    {
        currentEdit->setLineWrapMode(QTextEdit::WidgetWidth); // 开启自动换行
    }

    // 更新菜单项的选中状态
    form_autoline->setChecked(currentEdit->lineWrapMode() == QTextEdit::WidgetWidth);
}


