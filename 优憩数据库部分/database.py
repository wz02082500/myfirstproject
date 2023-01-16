# mysql数据库服务器，端口：3306，而且服务器是处于启动状态
# 安装pymysql：pip install pymysql
import pymysql
import threading


class DataManager():
    # 单例模式，确保每次实例化都调用一个对象。
    _instance_lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        if not hasattr(DataManager, "_instance"):
            with DataManager._instance_lock:
                DataManager._instance = object.__new__(cls)
                return DataManager._instance

        return DataManager._instance

    def __init__(self):
        # 建立连接
        """self.conn = pymysql.connect(host='rm-zzzzzzzzzzz.mysql.rds.aliyuncs.com', user='youqi', password='Youqi2023',
                               db='youqi', port=3306, charset='utf8')"""
        self.conn = pymysql.connect(host="",  user="root", passwd="lmr1313?L!", db="youqi")

        # 建立游标
        self.cursor = self.conn.cursor()
    #新建数据库
    def create_db(self):
        # 新建数据库
        self.cursor.execute('DROP TABLE IF EXISTS accounts')
        self.cursor.execute('DROP TABLE IF EXISTS data')

        sql1 = 'CREATE TABLE `data`(email_phone VARCHAR(45), `heart_rate` INT, `breathing` INT)'
        sql2 = 'CREATE TABLE `accounts` (name varchar(45) COLLATE utf8mb4_german2_ci NOT NULL,\
          `password` varchar(45) CHARACTER SET utf8mb4 COLLATE utf8mb4_german2_ci NOT NULL,\
          `email_phone` varchar(45) COLLATE utf8mb4_german2_ci NOT NULL,\
          PRIMARY KEY (`name`,`email_phone`)\
          ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_german2_ci;'

        self.cursor.execute(sql1)
        self.cursor.execute(sql2)
    #插入用户信息数据
    def insert_accounts(self, accounts):
        #accounts[0]="name", accounts[1]="email_phone", accounts[2]="password"
        sql = 'REPLACE INTO accounts (name, email_phone, password)\
                     values(%s,%s,%s)'
        try:
            self.cursor.execute(sql, accounts)
            self.conn.commit()
        except Exception as e:
            print('插入数据失败', e)
            self.conn.rollback()  # 回滚
    #插入用户睡眠状况数据
    def insert_data(self,data):
        sql= 'INSERT INTO data (email_phone, heart_rate, breathing)\
                     values(%s,%r,%r)'
        try:
            self.cursor.execute(sql, data)
            self.conn.commit()
        except Exception as e:
            print('插入数据失败', e)
            self.conn.rollback()  # 回滚
    #读取数据
    def read(self,email_phone):
        self.cursor.execute("SELECT heart_rate, breathing FROM youqi.data WHERE email_phone=\"" + str(email_phone) + "\"")
        data = self.cursor.fetchall()
        print("获取数据成功")
        if data != None:
            return (data)
        else:
            print("result：Null")
            return ([])
    #删除用户信息数据，包括用户登录信息和所有睡眠信息
    def delete_data(self,email_phone):
        sql1 = "DELETE FROM accounts WHERE email_phone=\"" + str(email_phone) + "\""
        sql2 = "DELETE FROM data WHERE email_phone=\"" + str(email_phone) + "\""
        try:
            self.cursor.execute(sql1)
            self.cursor.execute(sql2)
            self.conn.commit()
        except Exception as e:
            print('删除数据失败',e)
            self.conn.rollback()

    def __del__(self):
        # 关闭游标
        self.cursor.close()
        # 关闭连接
        self.conn.close()