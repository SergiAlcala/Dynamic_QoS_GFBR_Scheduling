class TimestampTag : public Tag
{
public:
  static TypeId GetTypeId()
  {
    static TypeId tid = TypeId("TimestampTag")
      .SetParent<Tag>()
      .AddConstructor<TimestampTag>();
    return tid;
  }

  TypeId GetInstanceTypeId() const override { return GetTypeId(); }

  void Serialize(TagBuffer i) const override
  {
    i.WriteDouble(m_timestamp.GetSeconds());
  }

  void Deserialize(TagBuffer i) override
  {
    m_timestamp = Seconds(i.ReadDouble());
  }

  uint32_t GetSerializedSize() const override { return sizeof(double); }

  void Print(std::ostream &os) const override
  {
    os << m_timestamp.GetSeconds();
  }

  void SetTimestamp(Time time) { m_timestamp = time; }
  Time GetTimestamp() const { return m_timestamp; }

private:
  Time m_timestamp;
};