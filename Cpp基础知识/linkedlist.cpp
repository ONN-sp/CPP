#include <iostream>

template<class T>
class Node{
    public:
        T data;
        Node* next;
        Node(T x):data(x), next(NULL){}
};

template<class T>
class LinkedList{
    private:
        Node<T>* head=new Node<T>(0);//不用new关键字创建,那么直接利用delete释放是不可行的
        int size;
    public:
        LinkedList():size(1){head->next=NULL;}
        ~LinkedList(){
            Node<T>* current=head;
            while(current!=NULL){
                Node<T>* next=current->next;
                delete current;
                current=next;
            }
        }
        
        void append(T data){
            Node<T>* newNode = new Node<T>(data);
            if(head->next==NULL){
                head->next=newNode;
            }
            else{
                Node<T>* current=head;
                while(current->next!=NULL)
                    current=current->next;
                current->next=newNode;
            }
            newNode->next=NULL;
            size++;
        }

        void display(){
            Node<T>* current = head;
            while(current!=NULL){
                std::cout << current->data << " ";
                current = current->next;
            }
            std::cout << "\n";
        }

        int getSize(){
            return size;
        }
};

int main() {
    LinkedList<int> list;
    list.append(1);
    list.append(2);
    list.append(3);
    list.display(); // Output: 1 2 3
    std::cout << "Size: " << list.getSize() << std::endl; // Output: Size: 3

    return 0;
}