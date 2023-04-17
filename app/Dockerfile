FROM python:3

LABEL maintainer="docki"

VOLUME /app/outputs
WORKDIR /usr/src/app

COPY .. /usr/src/app

RUN pip install --no-cache-dir -r requirements.txt

CMD ["python", "./multiply.py"]