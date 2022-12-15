from sqlalchemy import create_engine, Column, Integer, Float, DateTime
from sqlalchemy.orm import scoped_session, sessionmaker
from sqlalchemy.ext.declarative import declarative_base


engine = create_engine("sqlite:///database.db")
db_session = scoped_session(sessionmaker(bind=engine))
Base = declarative_base()
Base.query = db_session.query_property()


class Events(Base):
    __tablename__ = 'events'

    id = Column(Integer, primary_key=True, nullable=False)
    temperature = Column(Float)
    water_tank = Column(Float)
    heat_index = Column(Float)
    humedity = Column(Float)
    timestamp = Column(DateTime, nullable=False)


Base.metadata.create_all(
    engine, Base.metadata.tables.values(), checkfirst=True)
