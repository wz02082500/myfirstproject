# _*_ coding:utf-8 _*_
import os
import sys
import pymysql
from flask_cors import CORS
from flask import Flask, request, render_template,json
import random
from email.mime.text import MIMEText
from email.utils import formataddr
import smtplib

db = pymysql.connect(host="127.0.0.1", user="root", password="lmr1313?L!", db="youqi")
cursor = db.cursor()

#邮箱认证
TITLE = '优憩验证码'
USER = '1023613134@qq.com' #发件者邮箱
PASS = 'QFAQPLFQZRZBMVWQ' #授权码

# 后端服务启动  接受前端发送的请求
app = Flask(__name__)
CORS(app, resources=r'/*')  # 解决 flask跨域问题

#创建账号
@app.route('/accounts/addAccount', methods=['POST'])
def add_account():
    if request.method == "POST":
        # name = str(json.loads(request.values.get("name")))
        # password = str(json.loads(request.values.get("password")))
        # email = str(json.loads(request.values.get("email")))
        name = request.form.get("name")
        password = request.form.get("password")
        email = request.form.get("email")
        try:
            cursor.execute("INSERT INTO accounts(name,password,email) VALUES (\"" + str(name) + "\",\"" + str(
                password) + "\",\"" + str(email) + "\")")
            db.commit()
            print("add a new user successfully")
            res = '创建账号成功，请登录'
            return json.dumps(res.decode('utf8'))

            #return "创建账号成功，请登陆"
        except Exception as e:  # 打印错误信息
            print("add a new user failed:", e)
            db.rollback()
            res = '创建账号失败,检查是否重复创建'
            return json.dumps(res.decode('utf8'))
            #return "创建账号失败,检查是否重复创建"

#登录账号
@app.route('/accounts/login', methods=['POST'])
def users_login():
    if request.method == "POST":
        # email = str(json.loads(request.values.get("email")))
        # password = str(json.loads(request.values.get("password")))
        email = request.form.get("email")
        password = request.form.get("password")
        cursor.execute("SELECT email,password FROM accounts WHERE email=\"" + str(email) + "\"")
        data = cursor.fetchone()
        if data != None:
            if password == data[1]:
                print("login successfully,email:", email, "password:", password)
                return "登录成功"
            else:
                return "密码错误！"
        else:
            return "账号不存在"

# 生成六位数随机码
def getrate_random():
    list1 = []
    for i in range(6):
        unit = random.randint(0, 9)
        list1.append(unit)
    num = len(list1)
    m = ''  # 建立空字符串
    for i in range(num):
        x = str(list1[i])
        m = m + x
    return m

#发送邮件
@app.route('/accounts/sendEmail', methods=['POST'])
def send_email():
    if request.method == "POST":
        email = request.form.get("email")
        cursor.execute("SELECT email FROM accounts WHERE email=\"" + str(email) + "\"")
        receiver = cursor.fetchone()
        if receiver != None:
            """try:
                msg = MIMEText('这是邮件内容（请自行更改）', 'plain', 'utf-8')
                msg['From'] = formataddr(["youqi", USER])
                msg['To'] = formataddr(["FK", receiver])
                msg['Subject'] = "xxx的验证码（请自行更改）"
                print('已经设置好邮件信息')

                server = smtplib.SMTP_SSL("smtp.qq.com", 25)
                server.login(USER, PASS)
                server.sendmail(USER, [receiver, ], msg.as_string())
                server.quit()
                print('邮件发送已完成')
            except Exception as e:
                ret = False
                err = str(e)
                print('进入错误语句\n错误是%s' % (err))"""

            code = getrate_random()
            content = "您好，您的优憩账号验证码是： " + code
            mail_host = "smtp.qq.com"  # qq的SMTP服务器
            # 1.将邮件的信息打包成一个对象
            message = MIMEText(content, "plain", "utf-8")  # 内容，格式，编码
            # 2.设置邮件的发送者
            message["From"] = USER
            # 3.设置邮件的接收方
            # message["To"] = receiver
            # join():通过字符串调用，参数为一个列表
            message["To"] = ",".join(receiver)
            # 4.设置邮件的标题
            message["Subject"] = TITLE
            try:
                # 1.启用服务器发送邮件
                # 参数：服务器，端口号
                smtpObj = smtplib.SMTP()
                smtpObj.connect(mail_host, 25)  # 25 为 SMTP 端口号
                # 2.登录邮箱进行验证
                # 参数：用户名，授权码
                smtpObj.login(USER, PASS)
                # 3.发送邮件
                # 参数：发送方，接收方，邮件信息
                smtpObj.sendmail(USER, receiver, message.as_string())
                print("邮件发送成功，验证码为： ", code)
                smtpObj.close()
            except Exception as e:
                print('邮件发送失败', e)
                db.rollback()
                return "邮件发送失败，请检查是否正确填写邮箱"
            return "已向您填写的邮箱地址发送验证码"
        else:
            return "账号不存在"

#修改账号密码

#删除账号

#申请邮件发送

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8899)  # 0.0.0.0任意地址 port后端服务器提供服务的接口 监听端口8899
    db.close()
    print("finish")