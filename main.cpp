/* 
NAME: Adrian Konarski

Reading Device Kindle

User (USER CLASS) has library of books (class)
    - library

Library (CLASS - map)
    - map
    - activeBook ID
    + add book

Book (BOOK CLASS) is a sequence of charaters (class)
    - title
    - last page
    - total pages
    - bookContents

App (Main/Kindle device) (class)
    - tracks last page of books
    - tracks active book (1)
    - display page of book text
    - user can flip pages
*/

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
using namespace std;


/*
Book (BOOK CLASS) is a sequence of charaters (class)
    - title
    - last (current) page user is reading
    - bookContents
*/
class Book{
private:
    string title;
    string author;
    int bookID;
    string content;
    int currentPage;

    int calcBookID(string t, string a){
        return t.at(0) * a.at(1);
    }
public:
    Book(string title_, string author_, string content_, int page_){
        title = title_;
        author = author_;
        bookID = calcBookID(title_, author_);
        content = content_;
        currentPage = page_;
    }

    string getTitle(){return title;}
    string getAuthor(){return author;}
    string getContent(){return content;}
    int getBookID(){return bookID;}
};

class User{
private:
    string userName;

    //atm, 1 user = 1 library. 
    //move library to external class for many users => 1 book relationship 
    unordered_map<int, Book> user_library; 
    int active_book_id;

public:
    User(string name_){
        userName = name_;
    }
    string getUserName(){
        return userName;
    }
    void addBook(Book b){
        user_library.insert({b.getBookID(), b});
        active_book_id = b.getBookID(); //change later
    }

    string getBookTitle(){
        if(user_library.size() < 1) return "No books in library!";
        return user_library.at(active_book_id).getTitle();
    }

    string getBookContent(){
        if(user_library.size() < 1) return "No books in library!";
        return user_library.at(active_book_id).getContent();
    }

    void printLibrary(){
        for(auto& [key, book] : user_library){
            cout << book.getTitle() << " by " << book.getAuthor() << endl;
        }
    }
};

class Kindle{
private:
    vector<User> users;
    int currentUserIndex;
public:
    Kindle(){};
    void add_user(User u){
        users.push_back(u);
        if(users.size() == 1) currentUserIndex = 0;
    }
    void setCurrentUser(int id){
        if(id-1 > users.size()){currentUserIndex=0;}
        else {
            currentUserIndex = id-1;}
            cout<<"Swapping users to " << users.at(currentUserIndex).getUserName() <<endl;
    }
    string getCurrentUser(){
        if (currentUserIndex<0) return "No Users";
        return users.at(currentUserIndex).getUserName();
    }

    void addBook(Book b){
        users.at(currentUserIndex).addBook(b);
    }

    // get content -- display methods -- formatting/limiting content for page size/# lines missing
    string getContentTitle(){
        string content = users.at(currentUserIndex).getBookTitle();
        return content;
    }
    string getContent(){
        string content = users.at(currentUserIndex).getBookContent();
        return content;
    }
};

int main(){

    Kindle k;

    k.add_user(User("Adrian"));
    k.add_user(User("Mike"));
    cout << k.getCurrentUser() << endl;
    
    //swap to mike
    k.setCurrentUser(2);
    cout << k.getCurrentUser() << endl;
    k.addBook(Book("Title", "Mr. Author", "The lazy fox...", 1));
    cout << "Current content title: " << k.getContentTitle() << endl;
    cout << "Current book's content: " << k.getContent() << endl;

    //swap to adrian
    k.setCurrentUser(1);
    cout << "Current content title: " << k.getContent() << endl;

}