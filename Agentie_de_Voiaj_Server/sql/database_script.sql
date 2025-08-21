USE [master]
GO
/****** Object:  Database [Agentie_de_Voiaj]    Script Date: 8/21/2025 4:57:09 PM ******/
CREATE DATABASE [Agentie_de_Voiaj]
 CONTAINMENT = NONE
 ON  PRIMARY 
( NAME = N'Agentie_de_Voiaj', FILENAME = N'C:\Program Files\Microsoft SQL Server\MSSQL16.SQLEXPRESS\MSSQL\DATA\Agentie_de_Voiaj.mdf' , SIZE = 8192KB , MAXSIZE = UNLIMITED, FILEGROWTH = 65536KB )
 LOG ON 
( NAME = N'Agentie_de_Voiaj_log', FILENAME = N'C:\Program Files\Microsoft SQL Server\MSSQL16.SQLEXPRESS\MSSQL\DATA\Agentie_de_Voiaj_log.ldf' , SIZE = 8192KB , MAXSIZE = 2048GB , FILEGROWTH = 65536KB )
 WITH CATALOG_COLLATION = DATABASE_DEFAULT, LEDGER = OFF
GO
ALTER DATABASE [Agentie_de_Voiaj] SET COMPATIBILITY_LEVEL = 160
GO
IF (1 = FULLTEXTSERVICEPROPERTY('IsFullTextInstalled'))
begin
EXEC [Agentie_de_Voiaj].[dbo].[sp_fulltext_database] @action = 'enable'
end
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ANSI_NULL_DEFAULT OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ANSI_NULLS OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ANSI_PADDING OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ANSI_WARNINGS OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ARITHABORT OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET AUTO_CLOSE ON 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET AUTO_SHRINK OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET AUTO_UPDATE_STATISTICS ON 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET CURSOR_CLOSE_ON_COMMIT OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET CURSOR_DEFAULT  GLOBAL 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET CONCAT_NULL_YIELDS_NULL OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET NUMERIC_ROUNDABORT OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET QUOTED_IDENTIFIER OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET RECURSIVE_TRIGGERS OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET  ENABLE_BROKER 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET AUTO_UPDATE_STATISTICS_ASYNC OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET DATE_CORRELATION_OPTIMIZATION OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET TRUSTWORTHY OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ALLOW_SNAPSHOT_ISOLATION OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET PARAMETERIZATION SIMPLE 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET READ_COMMITTED_SNAPSHOT OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET HONOR_BROKER_PRIORITY OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET RECOVERY SIMPLE 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET  MULTI_USER 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET PAGE_VERIFY CHECKSUM  
GO
ALTER DATABASE [Agentie_de_Voiaj] SET DB_CHAINING OFF 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET FILESTREAM( NON_TRANSACTED_ACCESS = OFF ) 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET TARGET_RECOVERY_TIME = 60 SECONDS 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET DELAYED_DURABILITY = DISABLED 
GO
ALTER DATABASE [Agentie_de_Voiaj] SET ACCELERATED_DATABASE_RECOVERY = OFF  
GO
ALTER DATABASE [Agentie_de_Voiaj] SET QUERY_STORE = ON
GO
ALTER DATABASE [Agentie_de_Voiaj] SET QUERY_STORE (OPERATION_MODE = READ_WRITE, CLEANUP_POLICY = (STALE_QUERY_THRESHOLD_DAYS = 30), DATA_FLUSH_INTERVAL_SECONDS = 900, INTERVAL_LENGTH_MINUTES = 60, MAX_STORAGE_SIZE_MB = 1000, QUERY_CAPTURE_MODE = AUTO, SIZE_BASED_CLEANUP_MODE = AUTO, MAX_PLANS_PER_QUERY = 200, WAIT_STATS_CAPTURE_MODE = ON)
GO
USE [Agentie_de_Voiaj]
GO
/****** Object:  Table [dbo].[Destinations]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Destinations](
	[Destination_ID] [int] IDENTITY(1,1) NOT NULL,
	[Name] [varchar](100) NOT NULL,
	[Country] [varchar](100) NOT NULL,
	[Description] [text] NULL,
	[Image_Path] [varchar](255) NULL,
	[Date_Created] [datetime] NULL,
	[Date_Modified] [datetime] NULL,
PRIMARY KEY CLUSTERED 
(
	[Destination_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
/****** Object:  Table [dbo].[Types_of_Transport]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Types_of_Transport](
	[Transport_Type_ID] [int] IDENTITY(1,1) NOT NULL,
	[Name] [varchar](100) NOT NULL,
	[Description] [text] NULL,
	[Date_Created] [datetime] NULL,
	[Date_Modified] [datetime] NULL,
PRIMARY KEY CLUSTERED 
(
	[Transport_Type_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
/****** Object:  Table [dbo].[Accommodations]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Accommodations](
	[Accommodation_ID] [int] IDENTITY(1,1) NOT NULL,
	[Name] [varchar](100) NOT NULL,
	[Destination_ID] [int] NOT NULL,
	[Type_of_Accommodation] [int] NOT NULL,
	[Category] [varchar](10) NULL,
	[Address] [varchar](255) NULL,
	[Facilities] [text] NULL,
	[Rating] [decimal](4, 2) NULL,
	[Description] [text] NULL,
	[Date_Created] [datetime] NULL,
	[Date_Modified] [datetime] NULL,
PRIMARY KEY CLUSTERED 
(
	[Accommodation_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
/****** Object:  Table [dbo].[Offers]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Offers](
	[Offer_ID] [int] IDENTITY(1,1) NOT NULL,
	[Name] [varchar](150) NOT NULL,
	[Destination_ID] [int] NOT NULL,
	[Accommodation_ID] [int] NOT NULL,
	[Types_of_Transport_ID] [int] NOT NULL,
	[Price_per_Person] [decimal](10, 2) NOT NULL,
	[Duration_Days] [int] NOT NULL,
	[Departure_Date] [date] NOT NULL,
	[Return_Date] [date] NOT NULL,
	[Total_Seats] [int] NOT NULL,
	[Reserved_Seats] [int] NOT NULL,
	[Included_Services] [text] NULL,
	[Description] [text] NULL,
	[Status] [varchar](20) NOT NULL,
	[Date_Created] [datetime] NULL,
	[Date_Modified] [datetime] NULL,
PRIMARY KEY CLUSTERED 
(
	[Offer_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
/****** Object:  View [dbo].[v_Offers_Complete]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[v_Offers_Complete] AS
SELECT 
    o.Offer_ID, o.Name as Offer_Name, 
    d.Name as Destination, d.Country,
    a.Name as Accommodation_Name, a.Category, a.Rating,
    t.Name as Transport_Type,
    o.Price_per_Person, o.Duration_Days,
    o.Departure_Date, o.Return_Date,
    (o.Total_Seats - o.Reserved_Seats) as Available_Seats,
    o.Status
FROM Offers o
JOIN Destinations d ON o.Destination_ID = d.Destination_ID
JOIN Accommodations a ON o.Accommodation_ID = a.Accommodation_ID  
JOIN Types_of_Transport t ON o.Types_of_Transport_ID = t.Transport_Type_ID
WHERE o.Status = 'active';
GO
/****** Object:  Table [dbo].[Users]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Users](
	[User_ID] [int] IDENTITY(1,1) NOT NULL,
	[Username] [varchar](50) NOT NULL,
	[Password_Hash] [varchar](255) NOT NULL,
	[Password_Salt] [varchar](64) NOT NULL,
	[Email] [varchar](100) NULL,
	[First_Name] [varchar](50) NULL,
	[Last_Name] [varchar](50) NULL,
	[Phone] [varchar](15) NULL,
	[Date_Created] [datetime] NULL,
	[Date_Modified] [datetime] NULL,
PRIMARY KEY CLUSTERED 
(
	[User_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY],
UNIQUE NONCLUSTERED 
(
	[Username] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY],
UNIQUE NONCLUSTERED 
(
	[Email] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
GO
/****** Object:  Table [dbo].[Reservations]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Reservations](
	[Reservation_ID] [int] IDENTITY(1,1) NOT NULL,
	[User_ID] [int] NOT NULL,
	[Offer_ID] [int] NOT NULL,
	[Number_of_Persons] [int] NOT NULL,
	[Total_Price] [decimal](10, 2) NOT NULL,
	[Reservation_Date] [datetime] NULL,
	[Status] [varchar](20) NOT NULL,
	[Notes] [text] NULL,
PRIMARY KEY CLUSTERED 
(
	[Reservation_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
/****** Object:  View [dbo].[v_User_Reservations]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE VIEW [dbo].[v_User_Reservations] AS
SELECT 
    r.Reservation_ID, u.Username,
    o.Name as Offer_Name, d.Name as Destination,
    r.Number_of_Persons, r.Total_Price, r.Status,
    r.Reservation_Date
FROM Reservations r
JOIN Users u ON r.User_ID = u.User_ID
JOIN Offers o ON r.Offer_ID = o.Offer_ID
JOIN Destinations d ON o.Destination_ID = d.Destination_ID
JOIN Accommodations a ON o.Accommodation_ID = a.Accommodation_ID
JOIN Types_of_Transport t ON o.Types_of_Transport_ID = t.Transport_Type_ID
WHERE o.Status = 'active';
GO
/****** Object:  Table [dbo].[Reservation_Persons]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Reservation_Persons](
	[Reservation_Person_ID] [int] IDENTITY(1,1) NOT NULL,
	[Reservation_ID] [int] NOT NULL,
	[Full_Name] [varchar](100) NOT NULL,
	[CNP] [varchar](15) NOT NULL,
	[Birth_Date] [date] NOT NULL,
	[Person_Type] [varchar](20) NOT NULL,
PRIMARY KEY CLUSTERED 
(
	[Reservation_Person_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
GO
/****** Object:  Table [dbo].[Types_of_Accommodation]    Script Date: 8/21/2025 4:57:09 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Types_of_Accommodation](
	[Accommodation_Type_ID] [int] IDENTITY(1,1) NOT NULL,
	[Name] [varchar](100) NOT NULL,
	[Description] [text] NULL,
	[Date_Created] [datetime] NULL,
	[Date_Modified] [datetime] NULL,
PRIMARY KEY CLUSTERED 
(
	[Accommodation_Type_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
/****** Object:  Index [IX_Offers_Destination]    Script Date: 8/21/2025 4:57:10 PM ******/
CREATE NONCLUSTERED INDEX [IX_Offers_Destination] ON [dbo].[Offers]
(
	[Destination_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
GO
/****** Object:  Index [IX_Offers_Destination_Price]    Script Date: 8/21/2025 4:57:10 PM ******/
CREATE NONCLUSTERED INDEX [IX_Offers_Destination_Price] ON [dbo].[Offers]
(
	[Destination_ID] ASC,
	[Price_per_Person] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
GO
/****** Object:  Index [IX_Offers_Price]    Script Date: 8/21/2025 4:57:10 PM ******/
CREATE NONCLUSTERED INDEX [IX_Offers_Price] ON [dbo].[Offers]
(
	[Price_per_Person] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
GO
SET ANSI_PADDING ON
GO
/****** Object:  Index [IX_Offers_Status]    Script Date: 8/21/2025 4:57:10 PM ******/
CREATE NONCLUSTERED INDEX [IX_Offers_Status] ON [dbo].[Offers]
(
	[Status] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
GO
/****** Object:  Index [IX_Reservations_User]    Script Date: 8/21/2025 4:57:10 PM ******/
CREATE NONCLUSTERED INDEX [IX_Reservations_User] ON [dbo].[Reservations]
(
	[User_ID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
GO
SET ANSI_PADDING ON
GO
/****** Object:  Index [IX_Users_Username]    Script Date: 8/21/2025 4:57:10 PM ******/
CREATE NONCLUSTERED INDEX [IX_Users_Username] ON [dbo].[Users]
(
	[Username] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, OPTIMIZE_FOR_SEQUENTIAL_KEY = OFF) ON [PRIMARY]
GO
ALTER TABLE [dbo].[Accommodations] ADD  DEFAULT (getdate()) FOR [Date_Created]
GO
ALTER TABLE [dbo].[Accommodations] ADD  DEFAULT (getdate()) FOR [Date_Modified]
GO
ALTER TABLE [dbo].[Destinations] ADD  DEFAULT (getdate()) FOR [Date_Created]
GO
ALTER TABLE [dbo].[Destinations] ADD  DEFAULT (getdate()) FOR [Date_Modified]
GO
ALTER TABLE [dbo].[Offers] ADD  DEFAULT ((0)) FOR [Reserved_Seats]
GO
ALTER TABLE [dbo].[Offers] ADD  DEFAULT ('active') FOR [Status]
GO
ALTER TABLE [dbo].[Offers] ADD  DEFAULT (getdate()) FOR [Date_Created]
GO
ALTER TABLE [dbo].[Offers] ADD  DEFAULT (getdate()) FOR [Date_Modified]
GO
ALTER TABLE [dbo].[Reservations] ADD  DEFAULT (getdate()) FOR [Reservation_Date]
GO
ALTER TABLE [dbo].[Types_of_Accommodation] ADD  DEFAULT (getdate()) FOR [Date_Created]
GO
ALTER TABLE [dbo].[Types_of_Accommodation] ADD  DEFAULT (getdate()) FOR [Date_Modified]
GO
ALTER TABLE [dbo].[Types_of_Transport] ADD  DEFAULT (getdate()) FOR [Date_Created]
GO
ALTER TABLE [dbo].[Types_of_Transport] ADD  DEFAULT (getdate()) FOR [Date_Modified]
GO
ALTER TABLE [dbo].[Users] ADD  DEFAULT (getdate()) FOR [Date_Created]
GO
ALTER TABLE [dbo].[Users] ADD  DEFAULT (getdate()) FOR [Date_Modified]
GO
ALTER TABLE [dbo].[Accommodations]  WITH CHECK ADD FOREIGN KEY([Destination_ID])
REFERENCES [dbo].[Destinations] ([Destination_ID])
GO
ALTER TABLE [dbo].[Accommodations]  WITH CHECK ADD FOREIGN KEY([Type_of_Accommodation])
REFERENCES [dbo].[Types_of_Accommodation] ([Accommodation_Type_ID])
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD FOREIGN KEY([Accommodation_ID])
REFERENCES [dbo].[Accommodations] ([Accommodation_ID])
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD FOREIGN KEY([Destination_ID])
REFERENCES [dbo].[Destinations] ([Destination_ID])
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD FOREIGN KEY([Types_of_Transport_ID])
REFERENCES [dbo].[Types_of_Transport] ([Transport_Type_ID])
GO
ALTER TABLE [dbo].[Reservation_Persons]  WITH CHECK ADD FOREIGN KEY([Reservation_ID])
REFERENCES [dbo].[Reservations] ([Reservation_ID])
GO
ALTER TABLE [dbo].[Reservations]  WITH CHECK ADD FOREIGN KEY([Offer_ID])
REFERENCES [dbo].[Offers] ([Offer_ID])
GO
ALTER TABLE [dbo].[Reservations]  WITH CHECK ADD FOREIGN KEY([User_ID])
REFERENCES [dbo].[Users] ([User_ID])
GO
ALTER TABLE [dbo].[Accommodations]  WITH CHECK ADD CHECK  (([Rating]>=(0) AND [Rating]<=(10)))
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD CHECK  (([Reserved_Seats]<=[Total_Seats]))
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD CHECK  (([Reserved_Seats]>=(0)))
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD CHECK  (([Status]='expired' OR [Status]='inactive' OR [Status]='active'))
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD CHECK  (([Total_Seats]>(0)))
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD  CONSTRAINT [CK_Departure_Future] CHECK  (([Departure_Date]>=CONVERT([date],getdate())))
GO
ALTER TABLE [dbo].[Offers] CHECK CONSTRAINT [CK_Departure_Future]
GO
ALTER TABLE [dbo].[Offers]  WITH CHECK ADD  CONSTRAINT [CK_Price_Positive] CHECK  (([Price_per_Person]>(0)))
GO
ALTER TABLE [dbo].[Offers] CHECK CONSTRAINT [CK_Price_Positive]
GO
ALTER TABLE [dbo].[Reservations]  WITH CHECK ADD CHECK  (([Status]='cancelled' OR [Status]='paid' OR [Status]='confirmed' OR [Status]='pending'))
GO
ALTER TABLE [dbo].[Reservations]  WITH CHECK ADD  CONSTRAINT [CK_Persons_Positive] CHECK  (([Number_of_Persons]>(0)))
GO
ALTER TABLE [dbo].[Reservations] CHECK CONSTRAINT [CK_Persons_Positive]
GO
/****** Object:  StoredProcedure [dbo].[sp_Get_Destination_Stats]    Script Date: 8/21/2025 4:57:10 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_Get_Destination_Stats]
AS
BEGIN
    SELECT 
        d.Name as Destination,
        COUNT(o.Offer_ID) as Total_Offers,
        AVG(o.Price_per_Person) as Avg_Price,
        SUM(r.Number_of_Persons) as Total_Reservations
    FROM Destinations d
    LEFT JOIN Offers o ON d.Destination_ID = o.Destination_ID
    LEFT JOIN Reservations r ON o.Offer_ID = r.Offer_ID AND r.Status != 'cancelled'
    GROUP BY d.Destination_ID, d.Name
    ORDER BY Total_Reservations DESC
END
GO
USE [master]
GO
ALTER DATABASE [Agentie_de_Voiaj] SET  READ_WRITE 
GO
