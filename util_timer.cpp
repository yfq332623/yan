#include"util_timer.h"

void sort_timer_lst::add_timer(util_timer* timer){
    if(!head){
        head = tail = timer;
        return;
    }
    if(timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer,head);
}

void sort_timer_lst::adjust_timer(util_timer* timer){
    if(!timer){return;}

    util_timer* tem = timer->next;
    if(!tem||timer->expire < tem->expire){return;}

    if(timer==head){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer,head);
    }else{
        timer->prev->next = timer->next;
        timer->next->prev = timer ->prev;
        add_timer(timer,timer->next);
    }
    
}

void sort_timer_lst::add_timer(util_timer* timer , util_timer* lst_head){
    util_timer* pre = lst_head;
    util_timer* tem =lst_head->next;
    while(tem){
        if(timer->expire < tem->expire){
            pre->next = timer;
            timer->prev = pre;
            tem->prev = timer;
            timer->next = tem;
            break;
        }
        pre = tem;
        tem =tem->next;
    }
    if(!tem){
        pre->next = timer;
        timer->prev = pre;
        timer->next = nullptr;
        tail = timer;
    }
}

void sort_timer_lst::del_timer(util_timer* timer) {
    if (!timer) return;

    // 情况 1：链表只有一个节点，且正好是要删的这一个
    if ((timer == head) && (timer == tail)) {
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }

    // 情况 2：要删除的是头节点（且链表不止一个节点）
    if (timer == head) {
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }

    // 情况 3：要删除的是尾节点（且链表不止一个节点）
    if (timer == tail) {
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }

    // 情况 4：要删除的是中间节点
    // 把它前后的邻居互相“牵手”，把它架空
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

sort_timer_lst::~sort_timer_lst() {
    util_timer* tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

// 2. tick 函数：检查的核心动作
void sort_timer_lst::tick() {
    if (!head) return;
    
    // 获取当前系统时间
    time_t cur = time(NULL);
    util_timer* tmp = head;

    // 从头节点开始往后查，只要过期了就踢掉
    while (tmp) {
        // 升序链表：如果当前的还没过期，后面的肯定也没过期
        if (cur < tmp->expire) {
            break;
        }

        // 到期了，调用回调函数（踢人、写日志）
        if (tmp->cb_fun) {
            tmp->cb_fun(tmp->user_data);
        }

        // 处理完后，把该节点从链表中摘除并释放
        head = tmp->next;
        if (head) {
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}
    