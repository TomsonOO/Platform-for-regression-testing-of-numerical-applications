FROM quay.io/fenicsproject/stable:current

RUN pip install --no-cache-dir numpy pandas

COPY main.py /home/fenics/

WORKDIR /home/fenics/

CMD ["python", "main.py"]
