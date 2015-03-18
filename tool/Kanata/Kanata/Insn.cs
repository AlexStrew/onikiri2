using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.Serialization;
using System.IO;
using System.Collections.ObjectModel;

namespace Kanata
{
    public class Insn
    {
		public class Stage
        {
			private String comment = null;
			public String Comment 
			
			{
				get { return comment; } 
				set { comment = value; } 
			}

            public Int32 BeginRelCycle { get; set; }
			public Int32 Length { get; set; }
			public Int32 Id { get; set; }

            public int EndRelCycle
            {
                get
                {
                    return BeginRelCycle + Length;
                }
                set
                {
                    Length = (Int32)(value - BeginRelCycle);
                }
            }

            public void Read(BinaryReader r)
            {
                Id = r.ReadInt32();
                Length = r.ReadInt32();
                BeginRelCycle = r.ReadInt32();
            }

            public void Write(BinaryWriter w)
            {
                w.Write(Id);
                w.Write(Length);
                w.Write(BeginRelCycle);
            }
        }

		public class Relation
		{
			public ulong id;
			public int   type;
		};

        private List< List<Stage> > stageSegments = new List< List<Stage> >();

		private Relation[] producers;
		private String name;
		private String result;
		private ulong id;
        private long startCycle;
        private uint length;
        private ulong rid;
		private ulong tid;	// Thread ID
		private ulong gsid;	// Global serial ID
		private bool  flushed;

		private List<Relation> consumers = new List<Relation>();

        public Insn()
        {
        }

        /// <summary>
        /// Insn������������Bnew������Init��Read���Ă�
        /// </summary>
        public void Init()
        {
            name = "";
			result = "";
            id = 0;
            startCycle = 0;
			producers = new Relation[0];
            rid = 0;
            tid = 0;
			gsid = 0;
			flushed = false;
        }


        /// <summary>
        /// Insn��Id
        /// </summary>
        public ulong Id
        {
            get
            {
                return id;
            }
            set
            {
                id = value;
            }
        }

		/// <summary>
		/// Insn�̏o��
		/// </summary>
		public String Detail
		{
			get
			{
				return result;
			}
			set
			{
				result = value;
			}
		}
		
		/// <summary>
        /// Insn�̖��O�ALabel
        /// </summary>
        public String Name
        {
            get
            {
                return name;
            }
            set
            {
                name = value;
            }
        }


        /// <summary>
        /// Insn�̏������J�n����(�t�F�b�`���J�n����)�T�C�N��
        /// </summary>
        public long StartCycle
        {
            get
            {
                return startCycle;
            }
            set
            {
                startCycle = value;
            }
        }

        /// <summary>
        /// Insn�̏������I����(���^�C�A����)�T�C�N��
        /// </summary>
        public long EndCycle
        {
            get
            {
                return startCycle + length;
            }
            set
            {
                if (value < startCycle)
                    throw new ArgumentOutOfRangeException("EndCycle��BeginCycle�ȍ~�łȂ���΂����܂���");
                length = (uint)(value - startCycle);
            }
        }

        /// <summary>
        /// Insn�̏����ɂ����������Ԃ̒���
        /// </summary>
        public uint Length
        {
            get
            {
                return length;
            }
            set
            {
                length = value;
            }
        }

        /// <summary>
        /// stage
        /// </summary>
		public List< List<Stage> > StageSegments
        {
			get
			{
				return stageSegments;
			}
        }

        /// <summary>
        /// producer��Id
        /// </summary>
        public ReadOnlyCollection<Relation> Producers
        {
            get
            {
                return Array.AsReadOnly(producers);
            }
        }

        /// <summary>
        /// consumer��Id
        /// </summary>
		public ReadOnlyCollection<Relation> Consumers
        {
            get
            {
                return consumers.AsReadOnly();
            }
        }

        /// <summary>
        /// stage��ǉ�����Bpreprocessor�p�B
        /// </summary>
        /// <param name="s">�ǉ�����stage</param>
		public void AddStage( int segmentID, Stage s )
        {
			if( segmentID >= stageSegments.Count ) {
				for( int i = stageSegments.Count; i <= segmentID; i++ ) {
					stageSegments.Add( new List<Stage>() );
				}
			}
			stageSegments[segmentID].Add( s );
        }

        /// <summary>
        /// producer��ǉ�����Bpreprocessor�p�B
        /// </summary>
        /// <param name="aId">�ǉ�����producer��Id</param>
		public void AddProducer( Relation producer )
        {
			foreach( Relation i in producers ) {
                if ( i.id == producer.id )
                    return;
            }
            Array.Resize(ref producers, producers.Length + 1);
			producers[producers.Length - 1] = producer;
		}

        /// <summary>
        /// consumer��ǉ�����Bconsumer�͕ۑ�����Ȃ��B
        /// </summary>
        /// <param name="id">�ǉ�����consumer��Id</param>
        public void AddConsumer( Relation consumer )
        {
			consumers.Add( consumer );
        }

        /// <summary>
        /// Insn��RetireID
        /// </summary>
        public ulong Rid
        {
            get
            {
                return rid;
            }
            set
            {
                rid = value;
            }
        }

		/// <summary>
		/// Insn��Global serial ID
		/// </summary>
		public ulong Gsid
		{
			get
			{
				return gsid;
			}
			set
			{
				gsid = value;
			}
		}

		/// <summary>
		/// Insn��TID
		/// </summary>
		public ulong Tid
		{
			get
			{
				return tid;
			}
			set
			{
				tid = value;
			}
		}
		
		/// <summary>
        /// Insn���t���b�V�����ꂽ���ǂ���
        /// </summary>
        public bool Flushed
        {
            get
            {
                return flushed;
            }
            set
            {
				flushed = value;
            }
        }
    }
}
