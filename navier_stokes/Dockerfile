FROM quay.io/fenicsproject/stable:current

RUN pip install --no-cache-dir numpy pandas

COPY navier_stokes.py /home/fenics/

WORKDIR /home/fenics/

CMD ["python", "navier_stokes.py"]
