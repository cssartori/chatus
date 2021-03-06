/*
 * Developed by:
 * Carlo Sulzbach Sartori
 * Project done for "INF01151-Operating Systems II" class
 * 2015 - UFRGS
 *
 * The project is a Chat Client and Server in order to exercise the knowledge of
 * mutex/lock, semaphores, threads, concurrency and interprocess comunication.
 *
 * Originally, the was request for a program done in pure C, but to implement a Graphical User Interface the professor
 * allowed the implamentation to be done in C++ (strictly for the GUI).
 * That's the reason most of the core code is done using C notation and only the GUI uses OO. Also
 * some trickies had to be done in order to communicate between GUI and C pure code.
*/

#include "../include/client.h"
#include "../include/clientmainwindow.h"
#include "ui_clientmainwindow.h"
#include "../include/namedialog.h"
#include "../../protocol/include/protocol.h"
#include "../include/infomessages.h"

ClientMainWindow::ClientMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClientMainWindow)
{
    /*Automatic GUI setup (generated by Qt)*/
    ui->setupUi(this);

    /*Initially input functions are disabled - user can only send messages if inside a room*/
    this->inputEnabled(false);
    ui->roomList->clear();
    ui->leaveBtn->setText(LEAVE_CHAT_BTN_STRING);

    /*connect specific GUI menu options with respective actions*/
    connect(ui->saveRoomDialogAction, SIGNAL(triggered()), this, SLOT(save_room_dialog()));
    connect(ui->exitAction, SIGNAL(triggered()), this, SLOT(exit_event()));

    this->setWindowTitle(CHAT_WINDOW_TITLE);
}

ClientMainWindow::~ClientMainWindow()
{
    delete ui;
}

/*Set user name on GUI*/
void ClientMainWindow::set_user_name(const char *userName){
    ui->userNameLabel->setText(userName);
}

/*Enable or disable input fields: only allow input if user is in a room*/
void ClientMainWindow::inputEnabled(bool state) {
    ui->messageInputBox->setEnabled(state);
    ui->sendBtn->setEnabled(state);

    ui->messageOutputBox->clear();

    if(!state)
        ui->messageOutputBox->appendHtml(ROOT_ROOM_BASE_STRING);
}


/*Create Room actions*/
void ClientMainWindow::on_createRoomBtn_clicked()
{
    if(get_command_sent() == NO_COMMAND) {

        if(is_user_joined()) {
            /*Can't create a room if user is inside a room*/
            InfoMessages messageBox(this);
            messageBox.createRoomErrorMsg();
            return;
        }

        /*Dialog to enter new room name*/
        nameDIalog *roomNameDialog = new nameDIalog(this, "Insert Room Name: ");
        char new_room_name[MAX_ROOMNAME_SIZE];

        /*Starts dialog to get new room name*/
        if(! roomNameDialog->exec() ){
          /*entered a valid name*/
          strcpy(new_room_name, roomNameDialog->get_input_name());
          /*Sends request to server to create a new room*/
          user_create_room_request(new_room_name);
        }
    }
}

/*Leave actions: leave room or leave chat*/
void ClientMainWindow::on_leaveBtn_clicked()
{
    if(get_command_sent() == NO_COMMAND) {
        if(is_user_joined()) {
            /*user is in a room: leave the room*/
            /*Sends server request to leave the room*/
            user_leave_room_request();
        }
        else {
            /*User is outside of a room: Leave chat*/
            this->closeEvent(NULL);
        }
    }
}

/*Edit user name*/
void ClientMainWindow::on_editNameBtn_clicked()
{
    if(get_command_sent() == NO_COMMAND) {

        if(! is_user_joined()) {
            /*User can change his name*/
            /*Creates a Dialog to get new name input*/
            nameDIalog *nickNameDialog = new nameDIalog(this, "Insert New Nickname: ");
            char new_nickname[MAX_USERNAME_SIZE];

            /*Executes new name dialog*/
            if(! nickNameDialog->exec() ){
            /*entered a valid name*/
             strcpy(new_nickname, nickNameDialog->get_input_name());
             /*Sendo request to server for a new name*/
             user_change_nickname_request(new_nickname);
            }
        }
        else {
            /*User is in a room: Can't change user name*/
            InfoMessages messageBox(this);
            messageBox.changeNameMsg();
        }
    }
}

void ClientMainWindow::on_sendBtn_clicked()
{
    if(get_command_sent() == NO_COMMAND) {
        /*If there is nothing to send, return*/
        if(ui->messageInputBox->toPlainText().isEmpty())
            return;
        /*Sends request to server to broadcast message to all users in room*/
        user_message_request(ui->messageInputBox->toPlainText().toStdString().c_str());
        ui->messageInputBox->clear();
     }
}

/*Slot activated by signaler to set GUI to "user in room"*/
void ClientMainWindow::set_user_in_room(const char* room_name)
{
    this->set_room_name(room_name);
    this->clear_room_user_list();
    this->inputEnabled(true);
    ui->createRoomBtn->setEnabled(false);
    this->ui->leaveBtn->setText(LEAVE_ROOM_BTN_STRING);
}

/*Setup GUI when user leaves a room*/
void ClientMainWindow::pop_user_from_room()
{
    this->set_room_name("");
    this->clear_room_user_list();
    this->inputEnabled(false);
    ui->createRoomBtn->setEnabled(true);
    ui->leaveBtn->setText(LEAVE_CHAT_BTN_STRING);
}

/*Append message when a broadcast arrives*/
void ClientMainWindow::append_output_message(const char *message)
{
    ui->messageOutputBox->appendPlainText(message);
}

/*Set GUI room name*/
void ClientMainWindow::set_room_name(const char *room_name)
{
    char base_string[] = "<html><head/><body><p>Chat:  <span style=\" font-weight:600; text-decoration: underline;\">%s</span></p></body></html>";
    int base_len = strlen(base_string);
    char name[MAX_ROOMNAME_SIZE+base_len+1];
    sprintf(name,base_string, room_name);
    ui->chatLabel->setText(name);
}

/*Updates room list every time a broadcast of rooms arrive*/
void ClientMainWindow::update_room_list(const char* room_list)
{
    clear_room_user_list();
    QStringList rl = QString::fromLatin1(room_list).split("$", QString::SkipEmptyParts);

    foreach(QString room , rl) {
        ui->roomList->addItem(room);
    }
}

void ClientMainWindow::clear_room_user_list()
{
    ui->roomList->clear();
}

/*When users choses a item from the room list*/
void ClientMainWindow::on_roomList_doubleClicked(const QModelIndex &index)
{
    if(get_command_sent() == NO_COMMAND){

        if(is_user_joined())
            /*If user is joined (in a room) the list is the list of users in the room. Nothing to do.*/
            return;
        /*else, the list is the list of rooms opened. User is requesting a join to the room*/
        user_join_room_request(ui->roomList->item(index.row())->text().toStdString().c_str());
    }
}

void ClientMainWindow::user_name_exists_error(){
    InfoMessages messageBox(this);
    messageBox.nameExistsMsg();
}

void ClientMainWindow::room_full_error(){
    InfoMessages messageBox(this);
    messageBox.roomFullMsg();
}

void ClientMainWindow::room_space_error(){
    InfoMessages messageBox(this);
    messageBox.roomSpaceMsg();
}

void ClientMainWindow::room_name_exists_error(){
    InfoMessages messageBox(this);
    messageBox.roomNameExistsMsg();
}

void ClientMainWindow::room_not_found_error(){
    InfoMessages messageBox(this);
    messageBox.roomNotFoundMsg();
}


/*To save a room's dialog*/
void ClientMainWindow::save_room_dialog(){

    char* base_room_name = get_user_room_name();
    char room_name[strlen(base_room_name)+4];
    strcpy(room_name, base_room_name);
    strcat(room_name, ".txt");

    FILE *dialog_txt = fopen(room_name, "wt+");
    InfoMessages messageBox(this);
    if(dialog_txt == NULL) {
        messageBox.dialogFileErrorMsg();
        return;
    }

    fprintf(dialog_txt, "%s:\n%s", base_room_name, ui->messageOutputBox->toPlainText().toStdString().c_str());
    fclose(dialog_txt);
    messageBox.savedDialogMsg();
}

void ClientMainWindow::exit_event()
{
    this->closeEvent(NULL);
}


void ClientMainWindow::closeEvent(QCloseEvent *event){
    (void)event; //suppress unused variable warning (useless line)
    user_quit_request();
    while(is_user_connected());
    close_client();
    this->close();
}
