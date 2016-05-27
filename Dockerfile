FROM ubuntu:16.04

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y apt-file bash build-essential clang file
RUN apt-file update

ARG SRCDIR
COPY $SRCDIR /app

WORKDIR /app

RUN make CC="clang -W -Wall -Wextra -Werror" re
RUN ls -la
RUN file server
RUN file client

RUN make CC="gcc -W -Wall -Wextra -Werror" re
RUN ls -la
RUN file server
RUN file client

EXPOSE 6667

CMD [ "/app/server", "6667" ]