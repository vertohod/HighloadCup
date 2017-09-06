# Наследуемся от Ubuntu                                                                                                                                                                                                                       
FROM ubuntu                                                                                                                                                                                                                                   
WORKDIR /root                                                                                                                                                                                                                                 

USER root

RUN apt-get update
RUN apt-get install unzip -y --force-yes
RUN apt-get clean
RUN apt-get purge

ADD highloadcup /root/                                                                                                                                                                                                                        

# Открываем 80-й порт наружу                                                                                                                                                                                                                  
EXPOSE 80                                                                                                                                                                                                                                     
                                                                                                                                                                                                                                              
# Запускаем наш сервер                                                                                                                                                                                                                        
ENTRYPOINT ["/root/highloadcup", "80", "./data"]
