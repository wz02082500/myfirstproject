import pymysql
from database import DataManager

# 数据库链接   127。0.0.1回送地址,指本地机  cursor:数据库指针

#db = pymysql.connect(host="127.0.0.1", user="root", password="lmr1313?L!", db="youqi")
#cursor = db.cursor()

db = DataManager()

user1 = ["name", "123456@qq.com", "password"]
user1_data = ["123456@qq.com", 75, 23]
user2 = ["name2", "123@qq.com", "pass"]
user2_data = ["123@qq.com", 65, 25]
"""
db.insert_accounts(user1)
db.insert_data(user1_data)
db.insert_accounts(user2)
db.insert_data(user2_data)

db.delete_data(user2[1])
"""
data1 = db.read(user1[1])
print(data1)
print(user1[1])
